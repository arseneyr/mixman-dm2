<script lang="ts">
  import { onMount } from "svelte";
  import {
    ButtonId,
    MIDIStatus,
    sliderDefaults,
    SliderId,
    SliderStateType,
  } from "./constants";
  import Diagram from "./Diagram.svelte";

  export let inputPort: WebMidi.MIDIInput, outputPort: WebMidi.MIDIOutput;

  let inputMessage;
  let states: { buttons: Record<ButtonId, boolean>; sliders: SliderStateType } =
    {
      buttons: {},
      sliders: sliderDefaults(),
    };

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

  onMount(() => {
    inputPort.addEventListener("midimessage", inputMessageHandler);

    return () =>
      inputPort.removeEventListener("midiMessage", inputMessageHandler as any);
  });
</script>

<div class="container">{inputMessage}<Diagram {states} /></div>
