/// <reference types="@sveltejs/kit" />

declare module "*.svg" {
	import type { SvelteComponentTyped } from "svelte";
	export default class SvgComponent extends SvelteComponentTyped<SvgAttributes> { }
}