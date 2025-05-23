import { setupEventListeners } from "./dom.ts";
import { googleLetMeIn } from "./auth.ts";

setupEventListeners();

// @ts-expect-error
window.googleLetMeIn = googleLetMeIn;
