import App from "./AppWrapper.svelte";

const app = new App({
	target: document.getElementById("app")!,
});

export default app;