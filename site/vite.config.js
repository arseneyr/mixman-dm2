import { defineConfig } from "vite";
import { svelte } from "@sveltejs/vite-plugin-svelte";
import { svelteSVG } from "rollup-plugin-svelte-svg";

export default defineConfig({
  plugins: [svelteSVG({ enforce: "pre" }), svelte()],
});
