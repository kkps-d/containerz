"""Download PineconeMC's pinned vanilla Linux client into an offline data directory."""

import hashlib
import json
import shutil
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from urllib.request import urlopen


META_URL = "https://meta.pineconemc.ru/v1"
MINECRAFT = "net.minecraft"
LWJGL = "org.lwjgl3"
# Keep the launcher metadata stable even if the upstream indexes are updated.
COMPONENTS = {
    MINECRAFT: ("26.3", "eace936a849ae84c63f51ae12231b32715b4ee3cd8debf1ac06b0aab29f598bd"),
    LWJGL: ("3.4.3", "a7fb39cc0aaaf7c545667f570f3baaaf103e51974da45cf74dc92c112c083406"),
}


def download(url, path, checksum=None, size=None):
    path.parent.mkdir(parents=True, exist_ok=True)
    for attempt in range(3):
        try:
            with urlopen(url, timeout=30) as response, path.open("wb") as output:
                shutil.copyfileobj(response, output)
            break
        except OSError:
            if attempt == 2:
                raise
            time.sleep(1 << attempt)
    if size is not None and path.stat().st_size != size:
        raise ValueError(f"Unexpected download size for {url}")
    if checksum is not None:
        algorithm = "sha256" if len(checksum) == 64 else "sha1"
        with path.open("rb") as downloaded:
            if hashlib.file_digest(downloaded, algorithm).hexdigest() != checksum:
                raise ValueError(f"Checksum mismatch for {url}")
    return path


def metadata(root, name, checksum=None):
    path = download(f"{META_URL}/{name}.json", root / "meta" / f"{name}.json", checksum)
    return json.loads(path.read_text())


def supports_linux(library):
    if "rules" not in library:
        return True
    allowed = False
    for rule in library["rules"]:
        if rule.keys() != {"action", "os"} or rule["os"].keys() != {"name"}:
            raise ValueError(f"Unsupported library rule for {library['name']}: {rule}")
        if rule["os"]["name"] == "linux":
            allowed = rule["action"] == "allow"
    return allowed


def library_path(library):
    group, artifact, version, *classifier = library["name"].split(":")
    filename = "-".join((artifact, version, *classifier)) + ".jar"
    return Path("libraries", group.replace(".", "/"), artifact, version, filename)


def prepare(root):
    packages = {entry["uid"]: entry for entry in metadata(root, "index")["packages"]}
    components = {}
    for uid, (version, sha256) in COMPONENTS.items():
        index = metadata(root, f"{uid}/index", packages[uid]["sha256"])
        entry = next(entry for entry in index["versions"] if entry["version"] == version)
        if entry["sha256"] != sha256:
            raise ValueError(f"PineconeMC metadata changed for {uid} {version}")
        components[uid] = metadata(root, f"{uid}/{version}", sha256)

    game = components[MINECRAFT]
    if game["type"] != "release" or game["requires"] != [{"uid": LWJGL, "suggests": COMPONENTS[LWJGL][0]}]:
        raise ValueError("Unexpected dependencies for the pinned Minecraft release")

    assets = game["assetIndex"]
    index_path = download(
        assets["url"], root / "assets" / "indexes" / f"{assets['id']}.json", assets["sha1"], assets["size"]
    )
    downloads = {}
    for asset in json.loads(index_path.read_text())["objects"].values():
        digest = asset["hash"]
        address = f"{digest[:2]}/{digest}"
        downloads[Path("assets", "objects", address)] = (
            f"https://resources.download.minecraft.net/{address}", digest, asset["size"]
        )

    libraries = [game["mainJar"]]
    for component in components.values():
        libraries.extend(library for library in component["libraries"] if supports_linux(library))
    for library in libraries:
        artifact = library["downloads"]["artifact"]
        downloads[library_path(library)] = (artifact["url"], artifact["sha1"], artifact["size"])
    log_config = game["logging"]["file"]
    downloads[Path("assets", "log_configs", log_config["id"])] = (
        log_config["url"], log_config["sha1"], log_config["size"]
    )

    print(f"Preparing Minecraft {game['version']} with {len(downloads)} verified downloads", flush=True)
    with ThreadPoolExecutor(max_workers=24) as workers:
        futures = [
            workers.submit(download, url, root / path, checksum, size)
            for path, (url, checksum, size) in downloads.items()
        ]
        for future in futures:
            future.result()

    instance = root / "instance"
    (instance / ".minecraft").mkdir(parents=True)
    (instance / ".minecraft" / "options.txt").write_text("lang:en_us\n")
    java = "/usr/lib/jvm/java-25-openjdk-amd64/bin/java"
    (instance / "instance.cfg").write_text(
        "[General]\n"
        "ConfigVersion=1.3\n"
        "InstanceType=OneSix\n"
        f"name=Minecraft {game['version']}\n"
        "OverrideJavaLocation=true\n"
        f"JavaPath={java}\n"
        "AutomaticJava=false\n"
        "OverrideElySettings=true\n"
        "ElyPatchPreference=2\n"
    )
    pack = []
    for uid in (LWJGL, MINECRAFT):
        component = components[uid]
        entry = {"uid": uid, "version": component["version"], "cachedName": component["name"],
                 "cachedVersion": component["version"]}
        if uid == MINECRAFT:
            entry.update(important=True, cachedRequires=component["requires"])
        else:
            entry["dependencyOnly"] = True
        pack.append(entry)
    (instance / "mmc-pack.json").write_text(json.dumps({"formatVersion": 1, "components": pack}, indent=2) + "\n")
    (root / "elyprismlauncher.cfg").write_text(
        "[General]\n"
        f"JavaPath={java}\n"
        "IgnoreJavaWizard=true\n"
        "AutomaticJavaSwitch=false\n"
        "AutomaticJavaDownload=false\n"
        "UserAskedAboutAutomaticJavaDownload=true\n"
        "ApplicationTheme=system\n"
        "IconTheme=pe_colored\n"
        "Language=en_US\n"
        "PastebinURL=\n"
        "PineconeAutoServers=false\n"
    )


if __name__ == "__main__":
    prepare(Path(sys.argv[1]))
