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
	import MessageList from './MessageList.svelte';

	export let inputPort: WebMidi.MIDIInput, outputPort: WebMidi.MIDIOutput;
	export let svgElement;

	let messages = [];
	let states: { buttons: Record<ButtonId, boolean>; sliders: SliderStateType } = {
		buttons: {},
		sliders: sliderDefaults()
	};
	let enableLabels = false;

	$: enableLabels
		? svgElement.classList.remove('no-labels')
		: svgElement.classList.add('no-labels');

	function inputMessageHandler({ data }) {
		const [status, noteId, velocity] = data;
		if (status === MIDIStatus.Button) {
			states.buttons[noteId] = velocity > 0;
		} else if (status === MIDIStatus.Slider) {
			if (noteId === SliderId.WheelLeft || noteId === SliderId.WheelRight) {
				states.sliders[noteId] += velocity - 64;
			} else {
				states.sliders[noteId] = velocity;
			}
		}
		messages.unshift({ id: messages.length, sent: false, status, noteId, velocity });
		messages = messages;
	}

	function setLed(event: CustomEvent<{ id: number; state: boolean }>) {
		const noteId = event.detail.id;
		const velocity = event.detail.state ? LedState.On : LedState.Off;
		outputPort.send([MIDIStatus.LED, noteId, velocity]);
		messages.unshift({ id: messages.length, sent: true, status: MIDIStatus.LED, noteId, velocity });
		messages = messages;
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
<MessageList {messages} />

<style>
	label {
		user-select: none;
		padding-top: 8px;
	}
</style>
