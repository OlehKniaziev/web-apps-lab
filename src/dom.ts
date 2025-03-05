import type { Project, ProjectUpdateParams } from "./storage";
import { globalProjectRepository } from "./storage";

type NamedEventListener = {
    event: string;
    listener: EventListener;
};

type DynamicElement = {
    el: Element;
    listeners: NamedEventListener[];
};

const messageBox = querySelectorMust("#message-box");

const creationForm = querySelectorMust("#creation-form");
const updateForm = querySelectorMust("#update-form");

const projectListElem = querySelectorMust("#project-list");

const dynamicElements: DynamicElement[] = [
    {
        el: creationForm,
        listeners: [
            {
                event: "submit",
                listener: creationFormOnSubmit,
            },
        ],
    },
    {
        el: updateForm,
        listeners: [
            {
                event: "submit",
                listener: updateFormOnSubmit,
            },
        ],
    },

];

function querySelectorMust(selector: string): Element {
    const el = document.querySelector(selector);
    if (!el) {
        throw new Error(`Could not find an element conforming to selector '${selector}'`);
    }

    return el;
}

function getFormInputValue(form: HTMLFormElement, inputName: string): string {
    const input = form.elements.namedItem(inputName) as HTMLInputElement | null;
    if (!input) {
        throw new Error(`Could not find a named item with name '${name}'`);
    }

    return input.value;
};

function creationFormOnSubmit(ev: Event) {
    const event = ev as SubmitEvent;

    event.preventDefault();

    const form = event.target as HTMLFormElement | undefined;
    if (!form) {
        throw new Error("The event target is null!");
    }

    const projectName = getFormInputValue(form, "name");
    const projectDescription = getFormInputValue(form, "description");

    try {
        const proj = globalProjectRepository.createWithRandomID(projectName, projectDescription);
        displayMessage(`Successfully created a project with id '${proj.id}'`);
    } catch (e) {
        const error = e as Error;
        displayMessage(error.message, "error");
    }
}

function updateFormOnSubmit(ev: Event) {
    const event = ev as SubmitEvent;

    event.preventDefault();

    const form = event.target as HTMLFormElement | undefined;
    if (!form) {
        throw new Error("The event target is null!");
    }

    const oldProjectName = getFormInputValue(form, "old-name");
    const newProjectName = getFormInputValue(form, "new-name");
    const projectDescription = getFormInputValue(form, "description");

    const updateParams: ProjectUpdateParams = {
        oldName: oldProjectName,
        newName: newProjectName,
        description: projectDescription,
    };

    if (newProjectName === "") delete updateParams.newName;
    if (projectDescription === "") delete updateParams.description;

    globalProjectRepository.update(updateParams);
}

type MessageLevel = "info" | "error";

function displayMessage(message: string, level: MessageLevel = "info") {
    const className = `message-level-${level}`;
    messageBox.classList.value = className;
    messageBox.textContent = message;
}

function projectToElem(proj: Project): HTMLElement {
    const mainDiv = document.createElement("div");
    mainDiv.classList.add("project-item");

    const idP = document.createElement("p");
    const nameP = document.createElement("p");
    const descriptionP = document.createElement("p");

    idP.textContent = proj.id;
    nameP.textContent = proj.name;
    descriptionP.textContent = proj.description;

    const removeButton = document.createElement("button");
    removeButton.classList.add("button-danger");

    removeButton.textContent = "Remove";

    removeButton.addEventListener("click", () => globalProjectRepository.deleteByName(proj.name));

    mainDiv.append(idP, nameP, descriptionP, removeButton);

    return mainDiv;
}

export function initState() {
    const projects = globalProjectRepository.queryAll();

    for (const proj of projects) {
        const projAsElem = projectToElem(proj);
        projectListElem.appendChild(projAsElem);
    }
}

export function setupEventListeners() {
    for (const { el, listeners } of dynamicElements) {
        for (const { event, listener } of listeners) {
            el.addEventListener(event, listener);
        }
    }
}
