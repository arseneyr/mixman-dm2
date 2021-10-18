<script lang="ts">
  import App from "./App.svelte";

  import "./normalize.css";
  import "./skeleton.css";

  let inputPort = null,
    outputPort = null,
    error = null;

  (async () => {
    if (!navigator.requestMIDIAccess) {
      error = "WebMIDI not supported in your browser.";
      return;
    }
    try {
      const midiAccess = await navigator.requestMIDIAccess({ sysex: false });
      async function findPorts() {
        [inputPort, outputPort] = await Promise.all([
          Array.from(midiAccess.inputs.values())
            .find(
              ({ name, state }) =>
                name === "Mixman DM2" && state === "connected"
            )
            ?.open(),
          Array.from(midiAccess.outputs.values())
            .find(
              ({ name, state }) =>
                name === "Mixman DM2" && state === "connected"
            )
            ?.open(),
        ]);
        error =
          !inputPort || !outputPort
            ? "Please connect your Mixman DM2 device"
            : null;
      }
      midiAccess.addEventListener("statechange", findPorts);
      findPorts();
    } catch (e) {
      if (e.name === "InvalidAccessError") {
        error = "Could not access ports";
      } else {
        error = `Please allow MIDI permissions. Error: ${e.name}`;
      }
      return;
    }
  })();
</script>

{#if error}
  <h1>{error}</h1>
{:else if inputPort && outputPort}
  <App {inputPort} {outputPort} />
{/if}
