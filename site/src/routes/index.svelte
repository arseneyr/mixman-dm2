<script lang="ts">
	import { onMount } from 'svelte';
	import { browser } from '$app/env';
	import Svg from '$lib/dm2_diagram.svg';

	import Interactive from '$lib/Interactive.svelte';

	import '../app.css';
	import Subheading from '$lib/Subheading.svelte';
	import { SubheadingMessage } from '$lib/constants';

	let webMidiSupported = browser && !!navigator.requestMIDIAccess;

	let inputPort = null,
		outputPort = null,
		svgElement = null;

	let pluggedInOnce = false;

	let message = SubheadingMessage.NoJavascript;

	onMount(async () => {
		try {
			if (!webMidiSupported) {
				message = SubheadingMessage.WebMidiNotSupported;
				return;
			}
			message = null;
			const midiAccess = await navigator.requestMIDIAccess({ sysex: false });
			async function findPorts() {
				[inputPort, outputPort] = await Promise.all([
					Array.from(midiAccess.inputs.values())
						.find(({ name, state }) => name === 'Mixman DM2' && state === 'connected')
						?.open(),
					Array.from(midiAccess.outputs.values())
						.find(({ name, state }) => name === 'Mixman DM2' && state === 'connected')
						?.open()
				]);
				if (!inputPort || !outputPort) {
					message = pluggedInOnce ? SubheadingMessage.Unplugged : SubheadingMessage.NeverPluggedIn;
				} else {
					message = SubheadingMessage.PluggedIn;
					pluggedInOnce = true;
				}
			}
			midiAccess.addEventListener('statechange', findPorts);
			await findPorts();
		} catch (e) {
			message = SubheadingMessage.WebMidiNoPermissions;
		} finally {
			document.documentElement.classList.remove('js');
		}
	});
</script>

<svelte:head>
	<title>Mixman DM2 Demo</title>
	<script>
		document.documentElement.classList.add('js');
	</script>
	<style>
		html.js {
			visibility: hidden;
		}
	</style>
</svelte:head>

<div class="container">
	<h1>Mixman DM2 Demo</h1>
	<Subheading {message} />
	<Svg id="diagram" bind:svg={svgElement} />
	{#if svgElement && inputPort && outputPort}
		<Interactive {svgElement} {inputPort} {outputPort} />
	{/if}
</div>

<style>
	.container {
		height: 100%;
		display: flex;
		flex-direction: column;
	}
	:global(.container > *) {
		flex-shrink: 0;
	}
</style>
