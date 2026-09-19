import { defineConfig } from "vite";
import { copyFileSync, mkdirSync, readdirSync, existsSync } from "fs";

// Copy all top-level HTML pages and css/ folder into dist/ after build
function copyStaticFiles() {
  return {
    name: "copy-static-files",
    closeBundle() {
      // Copy HTML pages (except index.html which Vite handles)
      const htmlFiles = readdirSync(".")
        .filter((f) => f.endsWith(".html") && f !== "index.html");
      for (const f of htmlFiles) {
        if (existsSync(f)) copyFileSync(f, `dist/${f}`);
      }
      // Copy css/ folder so non-index pages can reference css/style.css
      if (!existsSync("dist/css")) mkdirSync("dist/css", { recursive: true });
      if (existsSync("css/style.css")) copyFileSync("css/style.css", "dist/css/style.css");
    },
  };
}

export default defineConfig({
  server: {
    port: 5173,
    host: true,
  },
  plugins: [copyStaticFiles()],
  optimizeDeps: {
    include: ['@supabase/supabase-js'],
  },
});
