<script lang="ts">
	import { onMount } from 'svelte';
	import { browser } from '$app/env';
	import Svg from '../dm2_diagram.svg';

	import Interactive from '../Interactive.svelte';

	import '../normalize.css';
	import '../skeleton.css';

	let webMidiSupported = browser && !!navigator.requestMIDIAccess;

	let inputPort = null,
		outputPort = null;

	let pluggedInOnce = false;

	let subheading = 'Enable Javascript to see a live demo!';

	onMount(async () => {
		try {
			if (!webMidiSupported) {
				subheading =
					'Open this page in Chrome or Edge (or another Chromium browser) to see a live demo!';
				return;
			}
			subheading = '';
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
					subheading = pluggedInOnce ? 'Plug in your DM2 turntable to see a live demo!' : 'install';
				} else {
					subheading = '';
					pluggedInOnce = true;
				}
			}
			midiAccess.addEventListener('statechange', findPorts);
			await findPorts();
		} catch (e) {
			subheading = 'Allow MIDI permissions to see a live demo!';
		} finally {
			document.documentElement.classList.remove('js');
		}
	});
</script>

<svelte:head>
	<link
		href="//fonts.googleapis.com/css?family=Raleway:400,300,600"
		rel="stylesheet"
		type="text/css"
	/>
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
	<p>
		{#if subheading === 'install'}
			Install the <a href="https://github.com/arseneyr/mixman-dm2">DM2 driver for Windows 10</a> and
			plug in your turntable for a live demo!
		{:else if subheading}
			{subheading}
		{/if}
	</p>
	{#if !webMidiSupported || !inputPort || !outputPort}
		<Svg />
	{:else}
		<Interactive {inputPort} {outputPort} />
	{/if}
</div>
