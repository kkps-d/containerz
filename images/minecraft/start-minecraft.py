#!/usr/bin/python3
"""Provision the default Minecraft instance, then run it under gamescope."""

import fcntl
import hashlib
import json
import os
import re
import shutil
import socket
import sys
import tempfile
import time
import uuid
from pathlib import Path


IDENTITY = ".containerz-offline-name"
LAUNCHER_CONFIG = "elyprismlauncher.cfg"
USERNAME = re.compile(r"[A-Za-z0-9_]{3,16}")


def atomic_write(path, write):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
        temporary = Path(stream.name)
        try:
            write(stream)
            stream.close()
            temporary.replace(path)
        finally:
            temporary.unlink(missing_ok=True)


def write_text(path, text):
    atomic_write(path, lambda stream: stream.write(text.encode("utf-8")))


def write_json(path, value):
    write_text(path, json.dumps(value, indent=2) + "\n")


def read_text(path):
    try:
        return path.read_text()
    except FileNotFoundError:
        return ""


def general_settings(lines):
    section = "General"
    settings = {}
    for index, line in enumerate(lines):
        if line.strip().startswith("["):
            section = line.strip()[1:-1]
        if section == "General" and "=" in line:
            key, value = line.split("=", 1)
            settings[key.strip()] = (index, value.strip())
    return settings


def update_settings(path, defaults, overrides=None):
    lines = read_text(path).splitlines()
    settings = general_settings(lines)
    updates = [f"{key}={value}" for key, value in defaults.items() if key not in settings]
    for key, value in (overrides or {}).items():
        if key in settings:
            index, previous = settings[key]
            if previous != value:
                lines[index] = f"{key}={value}"
        else:
            updates.append(f"{key}={value}")
    if updates:
        start = next((i + 1 for i, line in enumerate(lines) if line.strip() == "[General]"), 0)
        lines[start:start] = updates
    return "\n".join(lines) + "\n"


def copy_file(source, target):
    def copy(stream):
        with source.open("rb") as original:
            shutil.copyfileobj(original, stream)

    atomic_write(target, copy)


def merge_cache(seed, data):
    for directory in ("assets", "libraries", "meta"):
        for source in (seed / directory).rglob("*"):
            if not source.is_file():
                continue
            target = data / source.relative_to(seed)
            if not target.exists():
                copy_file(source, target)
            elif directory == "meta" and source.name == "index.json":
                key, entry_id = ("packages", "uid") if source.parent == seed / "meta" else ("versions", "version")
                current = json.loads(target.read_text())
                present = {entry[entry_id] for entry in current[key]}
                additions = [entry for entry in json.loads(source.read_text())[key] if entry[entry_id] not in present]
                if additions:
                    current[key].extend(additions)
                    write_json(target, current)
            elif not target.is_file():
                raise ValueError(f"Expected a Minecraft cache file at {target}")


def offline_account(path, name):
    # PineconeMC uses Java's UUID.nameUUIDFromBytes for OfflinePlayer:<name>.
    digest = hashlib.md5(f"OfflinePlayer:{name}".encode("ascii"), usedforsecurity=False).digest()
    profile_id = uuid.UUID(bytes=digest, version=3).hex
    existing = read_text(path)
    account_list = json.loads(existing) if existing or path.exists() else {"formatVersion": 3, "accounts": []}
    if account_list.get("formatVersion") != 3 or not isinstance(account_list.get("accounts"), list):
        raise ValueError(f"Unsupported PineconeMC account format in {path}")
    accounts = account_list["accounts"]
    for account in accounts:
        if account.get("profile", {}).get("id") == profile_id:
            if account.get("type") != "Offline" or account["profile"].get("name") != name:
                raise ValueError(f"Existing PineconeMC account conflicts with offline player {name}")
            return profile_id

    account = {
        "type": "Offline",
        "profile": {
            "id": profile_id,
            "name": name,
            "skin": {"id": "", "url": "", "variant": ""},
            "capes": [],
        },
        "ygg": {
            "token": "0",
            "iat": int(time.time()),
            "extra": {"userName": name, "clientToken": uuid.uuid4().hex},
        },
    }
    if not accounts:
        account["active"] = True
    accounts.append(account)
    write_json(path, account_list)
    return profile_id


def provision(seed, data, requested_name, instance_id):
    if not USERNAME.fullmatch(requested_name):
        raise ValueError("MINECRAFT_OFFLINE_NAME must be 3–16 letters, digits, or underscores")
    data.mkdir(mode=0o700, parents=True, exist_ok=True)
    with (data / ".containerz-setup.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        config = data / LAUNCHER_CONFIG
        existing_config = read_text(config)
        settings = {key: value for key, (_, value) in general_settings(existing_config.splitlines()).items()}
        instances = data / settings.get("InstanceDir", "instances").strip('"')
        instance = instances / instance_id
        managed = instance.exists() or instance.is_symlink()
        if managed and (instance.is_symlink() or not (instance / IDENTITY).is_file()):
            raise ValueError(f"Reserved instance path {instance} is already occupied")
        name = (instance / IDENTITY).read_text().strip() if managed else requested_name
        if not USERNAME.fullmatch(name):
            raise ValueError(f"Invalid offline identity in {instance / IDENTITY}")

        merge_cache(seed, data)
        profile_id = offline_account(data / "accounts.json", name)
        seed_config = (seed / LAUNCHER_CONFIG).read_text()
        if existing_config:
            defaults = {key: value for key, (_, value) in general_settings(seed_config.splitlines()).items()}
            # PineconeMC uses this value to notice when the data directory moves hosts.
            host = {"LastHostname": socket.gethostname()} if settings.get("IgnoreJavaWizard") == "false" else {}
            seed_config = update_settings(config, defaults, host)
        if seed_config != existing_config:
            write_text(config, seed_config)

        if managed:
            instance_config = instance / "instance.cfg"
            updated = update_settings(instance_config, {}, {"InstanceAccountId": profile_id})
            if updated != instance_config.read_text():
                write_text(instance_config, updated)
            return name

        staging = instances / ".tmp"
        staging.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=staging, prefix=f"{instance_id}-") as temporary:
            staged = Path(temporary) / instance_id
            shutil.copytree(seed / "instance", staged)
            instance_config = staged / "instance.cfg"
            instance_config.write_text(update_settings(instance_config, {}, {"InstanceAccountId": profile_id}))
            (staged / IDENTITY).write_text(name + "\n")
            staged.rename(instance)
        return name


def main():
    try:
        data = Path.home() / ".local/share/PineconeMC"
        seed = Path("/opt/minecraft-seed")
        pack = json.loads((seed / "instance" / "mmc-pack.json").read_text())
        release = next(component["version"] for component in pack["components"] if component["uid"] == "net.minecraft")
        instance_id = f"containerz-vanilla-{release}"
        name = provision(seed, data, os.environ.get("MINECRAFT_OFFLINE_NAME") or "Player", instance_id)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        sys.exit(f"start-minecraft: {error}")
    os.execvp("start-streamer", [
        "start-streamer", "env", "QT_QPA_PLATFORM=xcb", "start-pinecone",
        "--launch", instance_id, "--offline", name,
    ])


if __name__ == "__main__":
    main()
