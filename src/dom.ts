import type { Feature, FeaturePriority, FeatureState, Project, User, ProjectUpdateParams } from "./storage";
import { globalProjectRepository, globalUserRepository, globalFeatureRepository, createNewFeatureRightNow } from "./storage";

type NamedEventListener = {
    event: string;
    listener: EventListener;
};

type DynamicElement = {
    el: Element;
    listeners: NamedEventListener[];
};

let authFirstNameInput: HTMLInputElement | null = null;
let authLastNameInput: HTMLInputElement | null = null;
let authRoleInput: HTMLInputElement | null = null;

export function authGetFirstNameInput(): HTMLInputElement {
    if (authFirstNameInput === null) throw new Error("Input is null");
    return authFirstNameInput;
}

export function authGetLastNameInput(): HTMLInputElement {
    if (authLastNameInput === null) throw new Error("Input is null");
    return authLastNameInput;
}

export function authGetRoleInput(): HTMLInputElement {
    if (authRoleInput === null) throw new Error("Input is null");
    return authRoleInput;
}

const mainHeader = querySelectorMust("#main-header");

const messageBox = querySelectorMust("#message-box");

const creationForm = querySelectorMust("#creation-form");
const updateForm = querySelectorMust("#update-form");

const projectListElem = querySelectorMust("#project-list");

const activeProjectElem = querySelectorMust("#active-project");
const activeProjectFeatureListElem = querySelectorMust("#active-project-feature-list");

const newFeatureForm = querySelectorMust("#new-feature-form");

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
    {
        el: newFeatureForm,
        listeners: [
            {
                event: "submit",
                listener: newFeatureFormOnSubmit,
            },
        ],
    }
];

function querySelectorMust<T extends Element = Element>(selector: string): T {
    const el = document.querySelector<T>(selector);
    if (!el) {
        throw new Error(`Could not find an element conforming to selector '${selector}'`);
    }

    return el;
}

function getFormInputValue(form: HTMLFormElement, inputName: string): string {
    const input = form.elements.namedItem(inputName);
    if (!input) {
        throw new Error(`Could not find a named item with name '${inputName}'`);
    }

    if (input instanceof HTMLInputElement)
        return input.value;
    if (input instanceof HTMLSelectElement)
        return input.value;

    throw new Error(`Unsupported form input: ${input}`);
};

function creationFormOnSubmit(ev: Event) {
    const event = ev as SubmitEvent;

    event.preventDefault();

    const currentUser = globalUserRepository.getActive();
    if (currentUser.Role === "guest") {
        showErrorPopup("Not enough privileges to modify features");
        return;
    }

    const form = event.target as HTMLFormElement | undefined;
    if (!form) {
        throw new Error("The event target is null!");
    }

    const projectName = getFormInputValue(form, "name");
    const projectDescription = getFormInputValue(form, "description");

    try {
        const proj = globalProjectRepository.createWithRandomID(projectName, projectDescription);
        displayMessage(`Successfully created a project with id '${proj.Id}'`);
    } catch (e) {
        const error = e as Error;
        displayMessage(error.message, "error");
    }
}

async function updateFormOnSubmit(ev: Event) {
    const event = ev as SubmitEvent;

    event.preventDefault();

    const form = event.target as HTMLFormElement | undefined;
    if (!form) {
        throw new Error("The event target is null!");
    }

    const oldProjectName = getFormInputValue(form, "old-name");
    const newProjectName = getFormInputValue(form, "new-name");
    const projectDescription = getFormInputValue(form, "description");

    const project = await globalProjectRepository.queryByName(oldProjectName);
    if (!project) {
        throw new Error("No such project");
    }

    const updateParams: ProjectUpdateParams = {
        Id: project.Id,
        Name: newProjectName,
        Description: projectDescription,
    };

    if (newProjectName === "")     delete updateParams.Name;
    if (projectDescription === "") delete updateParams.Description;

    globalProjectRepository.update(updateParams);
}

function showErrorPopup(text: string) {
    const messageP = document.createElement("p");
    messageP.classList.add("message-level-error");
    messageP.textContent = text;
    showPopup(messageP);
}

function newFeatureFormOnSubmit(ev: Event) {
    const event = ev as SubmitEvent;

    event.preventDefault();

    const currentUser = globalUserRepository.getActive();
    if (currentUser.Role === "guest") {
        showErrorPopup("Not enough privileges to modify features");
        return;
    }

    const form = event.target as HTMLFormElement | undefined;
    if (!form) {
        throw new Error("The event target is null!");
    }

    const featureName = getFormInputValue(form, "name");
    const featureDescription = getFormInputValue(form, "description");
    const featurePriority = getFormInputValue(form, "priority");

    const activeProject = globalProjectRepository.getActive();
    if (!activeProject) throw new Error("No currently active project");
    const activeUser = globalUserRepository.getActive();
    if (!activeUser) throw new Error("No currently active user");

    if (featurePriority !== "low" && featurePriority !== "medium" && featurePriority !== "high")
        throw new Error(`Unsupported feature priority ${featurePriority}`);

    const newFeature = createNewFeatureRightNow({
        name: featureName,
        description: featureDescription,
        priority: featurePriority,
        owner: activeUser,
        project: activeProject,
    });

    globalFeatureRepository.addFeature(newFeature);
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

    idP.textContent = proj.Id;
    nameP.textContent = proj.Name;
    descriptionP.textContent = proj.Description;

    const removeButton = document.createElement("button");
    removeButton.classList.add("button", "button-danger");

    removeButton.textContent = "Remove";

    removeButton.addEventListener("click", () => globalProjectRepository.deleteByName(proj.Name));

    const setCurrentButton = document.createElement("button");
    setCurrentButton.classList.add("button", "button-action");

    setCurrentButton.textContent = "Set as current project";

    setCurrentButton.addEventListener("click", () => globalProjectRepository.setActive(proj));

    if (proj.Id === globalProjectRepository.getActive()?.Id) {
        const activeText = document.createElement("b");
        activeText.textContent = "[ACTIVE]";
        mainDiv.appendChild(activeText);
    }

    mainDiv.append(idP, nameP, descriptionP, removeButton, setCurrentButton);

    return mainDiv;
}

function updateActiveProjectViewHook(proj: Project): void {
    const table = document.createElement("table");
    const tHead = document.createElement("thead");
    const tBody = document.createElement("tbody");

    const headerRow = document.createElement("tr");

    const hId = document.createElement("th");
    const hName = document.createElement("th");
    const hDescr = document.createElement("th");

    hId.textContent = "Id";
    hName.textContent = "Name";
    hDescr.textContent = "Description";

    headerRow.append(hId, hName, hDescr);
    tHead.appendChild(headerRow);

    const bodyRow = document.createElement("tr");

    const idCell = document.createElement("td");
    const nameCell = document.createElement("td");
    const descrCell = document.createElement("td");

    idCell.textContent = proj.Id;
    nameCell.textContent = proj.Name;
    descrCell.textContent = proj.Description;

    bodyRow.append(idCell, nameCell, descrCell);
    tBody.appendChild(bodyRow);

    table.append(tHead, tBody);

    activeProjectElem.replaceChildren(table);
}

function updateMainHeaderHook(newUser: User): void {
    mainHeader.textContent = `Currently logged in as ${newUser.FirstName} ${newUser.LastName}.`;
}

function showPopup(...children: HTMLElement[]): HTMLElement {
    const target = document.body;

    const dialog = document.createElement("dialog");

    const closeButton = document.createElement("button");
    closeButton.classList.add("button");

    closeButton.textContent = "Close";

    closeButton.addEventListener("click", () => {
        dialog.close();
        target.removeChild(dialog);
    });

    dialog.append(...children, closeButton);

    target.appendChild(dialog);

    dialog.showModal();

    return closeButton
}

async function displayFeatureDetails(feat: Feature): Promise<void> {
    const featureElem = document.createElement("div");

    const title = document.createElement("h1");
    title.textContent = "Feature details";

    const p = (field: keyof Feature): HTMLElement => {
        const e = document.createElement("p");
        const fieldValue = feat[field] as string;
        e.textContent = `${field}: ${fieldValue}`;
        return e;
    };

    const pId = p("id");
    const pName = p("name");
    const pDescription = p("description");
    const pCreationDate = p("creationDate");

    const prioritySelect = document.createElement("select");

    const prio = (value: FeaturePriority): void => {
        const opt = document.createElement("option");
        opt.value = value;
        opt.textContent = value;
        if (feat.priority === value) opt.selected = true;
        prioritySelect.appendChild(opt);
    };

    prio("low");
    prio("medium");
    prio("high");

    prioritySelect.addEventListener("change", () => {
        feat.priority = prioritySelect.value as FeaturePriority;
        globalFeatureRepository.updateFeature(feat);
    });

    const project = await globalProjectRepository.queryByID(feat.projectID);
    if (!project) throw new Error(`No project with id ${feat.projectID}`);

    const pProject = document.createElement("p");
    pProject.textContent = `projectID: ${feat.projectID}(${project.Name})`;

    const owner = await globalUserRepository.queryByID(feat.ownerID);
    if (!owner) throw new Error(`No user with id ${feat.ownerID}`);

    const pOwner = document.createElement("p");
    pOwner.textContent = `ownerID: ${feat.ownerID}(${owner.FirstName} ${owner.LastName})`;

    const stateSelect = document.createElement("select");

    const state = (value: FeatureState): void => {
        const opt = document.createElement("option");
        opt.value = value;
        opt.textContent = value;
        if (feat.state === value) opt.selected = true;
        stateSelect.appendChild(opt);
    };

    state("todo");
    state("in-progress");
    state("done");

    stateSelect.addEventListener("change", () => {
        feat.state = stateSelect.value as FeatureState;
        globalFeatureRepository.updateFeature(feat);

        const currentProject = globalProjectRepository.getActive()!;
        renderProjectFeatures(currentProject);
    });

    featureElem.append(
        pId,
        pName,
        pDescription,
        pCreationDate,
        pProject,
        pOwner,
        prioritySelect,
        stateSelect,
    );

    showPopup(featureElem);
}

function renderProjectFeatures(proj: Project): void {
    const features = globalFeatureRepository.getProjectFeatures(proj);

    const table = document.createElement("table");
    const tHead = document.createElement("thead");
    const tBody = document.createElement("tbody");

    const todos = [];
    const doings = [];
    const dones = [];

    for (const feature of features) {
        switch (feature.state) {
            case "todo": todos.push(feature); break;
            case "in-progress": doings.push(feature); break;
            case "done": dones.push(feature); break;
        }
    }

    const maxLength = Math.max(todos.length, doings.length, dones.length);

    const appendFeatureIfExists = (row: HTMLElement, feature: Feature | undefined) => {
        const cell = document.createElement("td");
        if (feature) {
            cell.textContent = feature.name;
            cell.classList.add("pointer");
            cell.addEventListener("click", () => displayFeatureDetails(feature));
        }
        row.appendChild(cell);
    };

    for (let i = 0; i < maxLength; ++i) {
        const row = document.createElement("tr");
        appendFeatureIfExists(row, todos[i]);
        appendFeatureIfExists(row, doings[i]);
        appendFeatureIfExists(row, dones[i]);
        tBody.appendChild(row);
    }

    const hTodo = document.createElement("th");
    hTodo.textContent = "todo";
    const hDoing = document.createElement("th");
    hDoing.textContent = "doing";
    const hDone = document.createElement("th");
    hDone.textContent = "done";

    tHead.append(hTodo, hDoing, hDone);

    table.append(tHead, tBody);

    activeProjectFeatureListElem.replaceChildren(table);
}

async function renderAllProjects() {
    const projects = await globalProjectRepository.queryAll();
    const projectsElements = projects.map(projectToElem);
    projectListElem.replaceChildren(...projectsElements);
}

export function initState() {
    renderAllProjects();

    const currentUser = globalUserRepository.getActive();
    updateMainHeaderHook(currentUser);

    const currentProject = globalProjectRepository.getActive();
    // FIXME
    if (currentProject) {
        updateActiveProjectViewHook(currentProject);
        renderProjectFeatures(currentProject);
    }

    globalUserRepository.attachEventHook({
        type: "active-user-changed",
    }, updateMainHeaderHook);

    globalProjectRepository.attachEventHook({
        type: "active-project-changed",
    }, updateActiveProjectViewHook);

    globalProjectRepository.attachEventHook({
        type: "active-project-changed",
    }, () => renderAllProjects());

    globalProjectRepository.attachEventHook({
        type: "active-project-changed",
    }, renderProjectFeatures);
}

export function setupEventListeners() {
    for (const { el, listeners } of dynamicElements) {
        for (const { event, listener } of listeners) {
            el.addEventListener(event, listener);
        }
    }
}

function showLoginFormPopup() {
    const loginForm = document.createElement("form");

    const f = (name: string, desc: string, hidden: boolean = false) => {
        const input = document.createElement("input");
        input.name = name;
        input.id = name;
        input.placeholder = desc;
        input.style.display = hidden ? "none" : "inline";
        loginForm.appendChild(input);
    };

    f("first-name", "First name");
    f("last-name", "Last name");
    f("password", "Password");
    f("role", "Role", true);

    authFirstNameInput = loginForm.querySelector("#first-name");
    authLastNameInput = loginForm.querySelector("#last-name");
    authRoleInput = loginForm.querySelector("#role");

    authRoleInput!.value = "developer";

    const googleLoginTemplate = querySelectorMust<HTMLTemplateElement>("#google-login");
    const googleLogin = googleLoginTemplate.content.querySelectorAll("div");

    const loginButton = document.createElement("button");
    loginButton.textContent = "Login";

    const closeButton = showPopup(loginForm);

    loginButton.addEventListener("click", async (e) => {
        e.preventDefault();

        const firstName = getFormInputValue(loginForm, "first-name");
        const lastName = getFormInputValue(loginForm, "last-name");
        const password = getFormInputValue(loginForm, "password");
        const success = await globalUserRepository.loginUser(firstName, lastName, password);
        if (success) {
            initState();
            closeButton.click();
        } else {
            showErrorPopup("Invalid credentials.");
        }
    });

    const registerButton = document.createElement("button");
    registerButton.textContent = "Register";

    registerButton.addEventListener("click", async (e) => {
        e.preventDefault();

        const firstName = getFormInputValue(loginForm, "first-name");
        const lastName = getFormInputValue(loginForm, "last-name");
        const password = getFormInputValue(loginForm, "password");
        const role = getFormInputValue(loginForm, "role");

        if (!(role === "guest" || role === "admin" || role === "devops" || role === "developer")) {
            throw new Error(`Invalid role "${role}"`);
        }

        const success = await globalUserRepository.registerAndLoginUser(firstName, lastName, password, role);
        if (success) {
            initState();
            closeButton.click();
        } else {
            showErrorPopup("User with such credentials already exists.");
        }
    });

    closeButton.style.display = "none";

    const breakElem = document.createElement("br");

    loginForm.append(...googleLogin, breakElem, loginButton, registerButton);
}

showLoginFormPopup();
