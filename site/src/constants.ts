export const enum MIDIStatus {
	Button = 144,
	Slider = 176,
}

export const enum ButtonVelocity {
	Off = 0,
	On = 127
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

