<script lang="ts">
  import "./diagram.css";
  import { createEventDispatcher } from "svelte";

  import Svg from "../static/dm2_diagram.svg";
  import {
    ButtonId,
    SliderId,
    SliderStateType,
    WHEEL_COUNT_PER_ROTATION,
  } from "./constants";

  export let states: {
    buttons: Record<ButtonId, boolean>;
    sliders: SliderStateType;
  };

  export let enableLabels = false;

  let debouncers = {};

  function createDebouncer(el: HTMLElement) {
    let timer = null;
    return () => {
      timer && clearTimeout(timer);
      el.classList.add("slider-active");
      timer = setTimeout(() => el.classList.remove("slider-active"), 100);
    };
  }

  function debounce(el: HTMLElement, key: string | number) {
    if (!debouncers[key]) {
      debouncers[key] = createDebouncer(el);
    } else {
      debouncers[key]();
    }
  }

  const ledStates = Array.from({ length: 16 }, () => false);
  const dispatch = createEventDispatcher();
  function toggleLed(id: string) {
    dispatch("setLed", { id, state: (ledStates[id] = !ledStates[id]) });
  }

  $: ({
    buttons,
    sliders: {
      [SliderId.JoystickX]: joystickX,
      [SliderId.JoystickY]: joystickY,
      [SliderId.Slider]: slider,
      [SliderId.WheelLeft]: wheelLeft,
      [SliderId.WheelRight]: wheelRight,
    },
  } = states);

  let svgElement: SVGElement;

  function extractElements(labelPrefix: string): Record<string, HTMLElement> {
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
        return [labelSuffix, el];
      })
    );
  }

  $: buttonElements = svgElement && extractElements("button-");
  $: wheelElements = svgElement && extractElements("wheel-");
  let ledIcons: HTMLElement[] = [];

  $: ({ inner: joystickInner } = svgElement
    ? extractElements("joystick-")
    : {});
  $: ({ handle: sliderHandle } = svgElement ? extractElements("slider-") : {});

  $: {
    for (const [buttonNum, el] of Object.entries(buttonElements ?? {})) {
      const box = el.querySelector(".dm2-led-box");
      if (box) {
        box.addEventListener("click", (e) => toggleLed(buttonNum));
        box.addEventListener("mousedown", (e) => e.preventDefault());
        ledIcons[buttonNum] = el.querySelector(".dm2-led-icon");
      }
    }
  }

  $: ledIcons.forEach((el, i) =>
    ledStates[i]
      ? el.classList.add("led-active")
      : el.classList.remove("led-active")
  );

  $: {
    for (const [buttonNum, el] of Object.entries(buttonElements ?? {})) {
      if (buttons[buttonNum]) {
        el.classList.add("active-button");
      } else {
        el.classList.remove("active-button");
      }
    }
  }

  $: if (wheelElements) {
    wheelElements[SliderId.WheelLeft].style.setProperty(
      "transform",
      `rotate(${wheelLeft / WHEEL_COUNT_PER_ROTATION}turn)`
    );
    debounce(wheelElements[SliderId.WheelLeft], SliderId.WheelLeft);
  }

  $: if (wheelElements) {
    wheelElements[SliderId.WheelRight].style.setProperty(
      "transform",
      `rotate(${wheelRight / WHEEL_COUNT_PER_ROTATION}turn)`
    );
    debounce(wheelElements[SliderId.WheelRight], SliderId.WheelRight);
  }

  $: if (joystickInner) {
    joystickInner.style.setProperty(
      "transform",
      `translate(
        ${((joystickX - 64) / 64) * 40}%,
        ${-((joystickY - 64) / 64) * 40}%
      )`
    );
    debounce(joystickInner, "joystick");
  }
  $: if (sliderHandle) {
    sliderHandle.style.setProperty(
      "transform",
      `translateX(${((slider - 64) / 64) * 130}%)`
    );
    debounce(sliderHandle, "slider");
  }
</script>

<Svg
  id="diagram"
  class={enableLabels ? "" : "no-labels"}
  bind:svg={svgElement}
/>
