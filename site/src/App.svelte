<script lang="ts">
  import { onMount } from "svelte";
  import Diagram from "./Diagram.svelte";

  export let inputPort: WebMidi.MIDIInput, outputPort: WebMidi.MIDIOutput;

  let inputMessage;

  function inputMessageHandler({ data }) {
    inputMessage = data;
  }

  onMount(() => {
    inputPort.addEventListener("midimessage", inputMessageHandler);

    return () =>
      inputPort.removeEventListener("midiMessage", inputMessageHandler as any);
  });
</script>

<div class="container">{inputMessage}</div>
<Diagram />
