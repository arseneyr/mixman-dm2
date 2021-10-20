export const enum MIDIStatus {
	Button = 0x90,
	LED = 0x90,
	Slider = 0xB0,
}

export const enum ButtonVelocity {
	Off = 0,
	On = 0x7F
}

export type ButtonId = number;

export const enum SliderId {
	JoystickX = 0,
	JoystickY = 1,
	Slider = 2,
	WheelLeft = 3,
	WheelRight = 4
}

export const sliderDefaults = (): SliderStateType => ({
	[SliderId.JoystickX]: 64,
	[SliderId.JoystickY]: 64,
	[SliderId.Slider]: 64,
	[SliderId.WheelLeft]: 0,
	[SliderId.WheelRight]: 0
})

export type SliderStateType = {
	[K in typeof SliderId[keyof typeof SliderId]]: number;
};

export const enum LedState {
	Off = 0,
	On = 0x7f
}

export const WHEEL_COUNT_PER_ROTATION = 3000;
