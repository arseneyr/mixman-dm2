<script lang="ts">
	import { ButtonVelocity, LedState, MIDIStatus, toHexString } from './constants';
	import { fade } from 'svelte/transition';
</script>

<div transition:fade={{ duration: 100 }}>
	<h4>Incoming Messages</h4>

	<table>
		<thead>
			<tr>
				<th />
				<th>Byte 0 - Status</th>
				<th>Byte 1 - Button/Slider ID<br /><i>See Above</i></th>
				<th>Byte 2 - Velocity</th>
			</tr>
		</thead>
		<tbody>
			<tr>
				<th>Buttons</th>
				<td class="mono">
					{toHexString(MIDIStatus.Button)}
				</td>
				<td style="font-style: italic">
					{`<Button ID>`}
				</td>
				<td>
					<ul>
						<li>
							<span class="mono">{toHexString(ButtonVelocity.On)}</span> - Pressed
						</li>
						<li>
							<span class="mono">{toHexString(ButtonVelocity.Off)}</span> - Released
						</li>
					</ul>
				</td>
			</tr>
			<tr>
				<th>Slider and Joystick</th>
				<td class="mono">{toHexString(MIDIStatus.Slider)}</td>
				<td style="font-style: italic">{`<Slider/Joystick Axis ID>`}</td>
				<td class="mono">[0x00, 0x7F]</td>
			</tr>
			<tr>
				<th>Wheels</th>
				<td class="mono">{toHexString(MIDIStatus.Slider)}</td>
				<td style="font-style: italic">{`<Wheel ID>`}</td>
				<td
					><ul>
						<li><span class="mono">[0x00, 0x3F]</span> - Turning counterclockwise</li>
						<li><span class="mono">0x40</span> - Not Moving</li>
						<li><span class="mono">[0x41, 0x7F]</span> - Turning clockwise</li>
					</ul></td
				>
			</tr>
		</tbody>
	</table>
	<h4>Outgoing Messages</h4>

	<table>
		<thead>
			<tr>
				<th />
				<th>Byte 0 - Status</th>
				<th>Byte 1 - LED ID<br /><i>Same as button ID, see above</i></th>
				<th>Byte 2 - Data</th>
			</tr>
		</thead>
		<tbody>
			<tr>
				<th>LEDs</th>
				<td class="mono">
					{toHexString(MIDIStatus.LED)}
				</td>
				<td style="font-style: italic">
					{`<LED ID>`}
				</td>
				<td>
					<ul>
						<li>
							<span class="mono">{toHexString(LedState.On)}</span> - On
						</li>
						<li>
							<span class="mono">{toHexString(LedState.Off)}</span> - Off
						</li>
					</ul>
				</td>
			</tr>
		</tbody>
	</table>
</div>

<style>
	div {
		padding-top: 16px;
	}
	.mono {
		font-family: 'Space Mono', monospace;
	}
	h4 {
		margin-bottom: 8px;
	}
	ul {
		margin: 0;
	}
</style>
