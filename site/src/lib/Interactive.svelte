<script lang="ts">
	import { onMount } from 'svelte';
	import {
		ButtonId,
		LedState,
		MIDIStatus,
		sliderDefaults,
		SliderId,
		SliderStateType
	} from './constants';
	import Diagram from './InteractiveDiagram.svelte';

	export let inputPort: WebMidi.MIDIInput, outputPort: WebMidi.MIDIOutput;
	export let svgElement;

	let inputMessage;
	let states: { buttons: Record<ButtonId, boolean>; sliders: SliderStateType } = {
		buttons: {},
		sliders: sliderDefaults()
	};
	let enableLabels = false;

	$: enableLabels
		? svgElement.classList.remove('no-labels')
		: svgElement.classList.add('no-labels');

	function inputMessageHandler({ data }) {
		inputMessage = data;
		const [status, id, velocity] = data;
		if (status === MIDIStatus.Button) {
			states.buttons[id] = velocity > 0;
		} else if (status === MIDIStatus.Slider) {
			if (id === SliderId.WheelLeft || id === SliderId.WheelRight) {
				states.sliders[id] += velocity - 64;
			} else {
				states.sliders[id] = velocity;
			}
		}
	}

	function setLed(event: CustomEvent<{ id: number; state: boolean }>) {
		outputPort.send([
			MIDIStatus.LED,
			event.detail.id,
			event.detail.state ? LedState.On : LedState.Off
		]);
	}

	onMount(() => {
		inputPort.addEventListener('midimessage', inputMessageHandler);

		return () => {
			inputPort.removeEventListener('midiMessage', inputMessageHandler as any);
			svgElement.classList.remove('no-labels');
		};
	});
</script>

<Diagram {states} on:setLed={setLed} {svgElement} />
<label>
	<input type="checkbox" bind:checked={enableLabels} />
	<span class="label-body">Show Labels</span>
</label>

<style>
	label {
		user-select: none;
	}
</style>
