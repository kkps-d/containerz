package org.prismlauncher;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.nio.file.attribute.PosixFilePermission;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

final class VideoOptions {
    static void apply() throws IOException {
        String gameDir = System.getenv("INST_MC_DIR");
        if (gameDir == null || gameDir.isEmpty()) {
            throw new IOException("PineconeMC did not supply the instance directory");
        }
        String requested = System.getenv("DISPLAY_FPS");
        int fps = requested == null ? 60 : Integer.parseInt(requested);
        if (fps <= 0) {
            throw new IOException("DISPLAY_FPS must be positive");
        }
        apply(Paths.get(gameDir, "options.txt"), fps);
        System.out.println("Containerz: Minecraft maxFps=" + fps + ", VSync enabled");
    }

    private static boolean isVideoOption(String line) {
        return line.startsWith("maxFps:") || line.startsWith("enableVsync:");
    }

    private static void apply(Path options, int fps) throws IOException {
        if (Files.isSymbolicLink(options)) {
            options = options.toRealPath();
        }

        List<String> existing;
        Set<PosixFilePermission> permissions = null;
        try {
            existing = Files.readAllLines(options);
            permissions = Files.getPosixFilePermissions(options);
        } catch (NoSuchFileException missing) {
            existing = Collections.emptyList();
        }

        String cap = "maxFps:" + fps;
        String vsync = "enableVsync:true";
        if (existing.stream().filter(VideoOptions::isVideoOption).count() == 2
                && existing.contains(cap) && existing.contains(vsync)) {
            return;
        }

        List<String> updated = new ArrayList<>(existing);
        updated.removeIf(VideoOptions::isVideoOption);
        updated.add(cap);
        updated.add(vsync);

        Path temporary = Files.createTempFile(options.getParent(), ".options-", ".txt");
        try {
            Files.write(temporary, updated);
            if (permissions != null) {
                Files.setPosixFilePermissions(temporary, permissions);
            }
            Files.move(temporary, options, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
        } finally {
            Files.deleteIfExists(temporary);
        }
    }
}
