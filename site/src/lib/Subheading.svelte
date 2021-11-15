<script lang="ts">
	import { SubheadingMessage } from './constants';
	import { fade } from 'svelte/transition';

	export let message: SubheadingMessage | null;
</script>

<div class="container">
	{#if message === SubheadingMessage.NoJavascript}
		<div>Enable Javascript to see a live demo!</div>
	{:else if message === SubheadingMessage.WebMidiNotSupported}
		<div>Open this page in Chrome or Edge (or another Chromium browser) to see a live demo!</div>
	{:else if message === SubheadingMessage.WebMidiNoPermissions}
		<div>Allow MIDI permissions to see a live demo!</div>
	{:else if message === SubheadingMessage.NeverPluggedIn}
		<div out:fade={{ duration: 100 }}>
			Install the <a href="https://github.com/arseneyr/mixman-dm2">Windows 10 Mixman DM2 Driver</a> and
			plug your turntable in to see a live demo!
		</div>
	{:else if message === SubheadingMessage.Unplugged}
		<div transition:fade={{ duration: 100 }}>Plug in your turntable to see a live demo!</div>
	{:else if message === SubheadingMessage.PluggedIn}
		<div transition:fade={{ duration: 100 }}><b>Connected to turntable!</b></div>
	{/if}
</div>

<style>
	div.container {
		position: relative;
		width: 100%;
		height: 5rem;
	}
	div.container > div {
		position: absolute;
		left: 4px;
	}
</style>
