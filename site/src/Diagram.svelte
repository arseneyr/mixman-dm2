<script lang="ts">
  import Svg from "../static/dm2_diagram_1.svg";
  import { ButtonId, SliderId, SliderStateType } from "./constants";

  export let states: {
    buttons: Record<ButtonId, boolean>;
    sliders: SliderStateType;
  };

  $: ({ buttons, sliders } = states);

  let svgElement: SVGElement;

  function extractElements(
    labelPrefix: string
  ): Record<string | number, HTMLElement> {
    const els = Array.from(
      svgElement.querySelectorAll<HTMLElement>(
        `[inkscape\\:label^=${labelPrefix}]`
      )
    );
    return Object.fromEntries(
      els.map((el) => {
        const labelSuffix = el
          .getAttribute("inkscape:label")
          .substring(labelPrefix.length);
        return [parseInt(labelSuffix) || labelSuffix, el];
      })
    );
  }

  $: buttonElements = svgElement && extractElements("button-");
  $: wheelElements = svgElement && extractElements("wheel-");
  $: ({ inner: joystickInner } = svgElement
    ? extractElements("joystick-")
    : {});
  $: ({ handle: sliderHandle } = svgElement ? extractElements("slider-") : {});

  $: {
    for (const [buttonNum, el] of Object.entries(buttonElements ?? {})) {
      if (buttons[buttonNum]) {
        el.classList.add("active-button");
      } else {
        el.classList.remove("active-button");
      }
    }
  }

  $: {
    for (const [wheelNum, el] of Object.entries(wheelElements ?? {})) {
      sliders[wheelNum] &&
        el.style.setProperty(
          "transform",
          `rotate(${sliders[wheelNum] / 3000}turn)`
        );
    }
  }

  $: if (joystickInner) {
    joystickInner.style.setProperty(
      "transform",
      `translate(
        ${((sliders[SliderId.JoystickX] - 64) / 64) * 40}%,
        ${-((sliders[SliderId.JoystickY] - 64) / 64) * 40}%
      )`
    );
  }
  $: if (sliderHandle) {
    sliderHandle.style.setProperty(
      "transform",
      `translateX(${((sliders[SliderId.Slider] - 64) / 64) * 130}%)`
    );
  }
</script>

<Svg bind:svg={svgElement} style="width: 100%; height: auto" />

<style>
  :global(svg .active-button .dm2-button, svg .active-button.dm2-button) {
    fill: red;
  }
  :global(svg [inkscape\:label^="wheel-"]) {
    transform-origin: center;
    transform-box: fill-box;
  }
  :global(svg [inkscape\:label="joystick-inner"], svg
      [inkscape\:label="slider-handle"]) {
    transform-box: fill-box;
  }
</style>
