<script lang="ts">
	import { afterUpdate, tick } from 'svelte';
	import UploadArrow from './upload.svg';

	import { MIDIStatus } from './constants';

	export let messages: {
		id: number;
		sent: boolean;
		status: MIDIStatus;
		noteId: number;
		velocity: number;
	}[];

	let wrapperEl: HTMLDivElement;
	let childHeight;

	afterUpdate(async () => {
		if (!wrapperEl) {
			return;
		}
		if (!childHeight) {
			const row = wrapperEl.querySelector('tr');
			if (!row) {
				return;
			}
			childHeight = row.offsetHeight;
		}
		wrapperEl.scroll({ top: childHeight, behavior: 'auto' });
		await tick();
		wrapperEl.scroll({ top: 0, behavior: 'smooth' });
	});
</script>

<div class="wrapper" bind:this={wrapperEl}>
	<table>
		<tbody>
			{#each messages as message (message.id)}
				<tr>
					<td class="sent">
						{#if message.sent}
							<UploadArrow />
						{/if}
					</td>
					<td>
						<div class="mono">0x{message.status.toString(16).toUpperCase()}</div>
						<div class="label">Status</div>
					</td>
					<td>
						<div class="mono">{message.noteId.toString().padStart(2, '0')}</div>
						<div class="label">
							{message.sent ? 'LED' : message.status === MIDIStatus.Button ? 'Button' : 'Slider'}
						</div>
					</td>
					<td>
						<div class="mono">0x{message.velocity.toString(16).toUpperCase().padStart(2, '0')}</div>
						<div class="label">
							{message.status === MIDIStatus.Slider || message.sent ? 'Data' : 'Velocity'}
						</div>
					</td>
				</tr>
			{/each}</tbody
		>
	</table>
</div>

<style>
	.wrapper {
		overflow-y: auto;
		flex-shrink: 1;
		align-self: end;
	}
	table {
		overflow-y: auto;
		margin: 0px 24px;
	}
	td {
		border: none;
		padding: 8px 32px;
	}
	.mono {
		font-family: 'Space Mono', monospace;
		font-size: 24px;
	}
	.label {
		opacity: 0.7;
		line-height: 0.5;
	}
	.sent {
		fill: rgb(99, 99, 99);
		width: 24px;
		padding: 8px 0px 0px 0px;
	}
</style>
