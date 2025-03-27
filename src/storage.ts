export type Project = {
    id: string;
    name: string;
    description: string;
};

export type ProjectUpdateParams = {
    oldName: string;
    newName?: string | undefined;
    description?: string | undefined;
};

type ProjectEventSelector = {
    type: "active-project-changed";
};

type ProjectEventHook = (proj: Project) => void;

interface ProjectRepository {
    createWithRandomID(name: string, description: string): Project;
    queryAll(): Project[];
    queryByName(name: string): Project | null;
    queryByID(id: string): Project | null;
    deleteByName(name: string): void;
    deleteByID(id: string): void;
    update(params: ProjectUpdateParams): void;
    setActive(project: Project): void;
    getActive(): Project | undefined;
    attachEventHook(selector: ProjectEventSelector, hook: ProjectEventHook): void;
}

type ProjectRepositoryMutator = {
    setActiveProject(proj: Project): void;
    addProject(proj: Project): void;
    removeProject(projectID: string): void;
};

class ProjectRepositoryState {
    private activeProject?: Project;
    private projects: Project[] = [];

    public constructor() {
        const existingState = localStorage.getItem(ProjectRepositoryState.LOCAL_STORAGE_KEY);

        if (!existingState) {
            const thisString = JSON.stringify(this);
            localStorage.setItem(ProjectRepositoryState.LOCAL_STORAGE_KEY, thisString);
        } else {
            const state = JSON.parse(existingState) as ProjectRepositoryState;
            this.activeProject = state.activeProject;
            this.projects = state.projects;
        }
    }

    public modify(cb: (mutator: ProjectRepositoryMutator) => void): void {
        const state = this;

        const mutator: ProjectRepositoryMutator = {
            setActiveProject(proj: Project): void {
                state.activeProject = proj;
            },
            addProject(proj: Project): void {
                state.projects.push(proj);
            },
            removeProject(projectID: string): void {
                for (let i = 0; i < state.projects.length; ++i) {
                    if (state.projects[i].id === projectID) {
                        state.projects.splice(i, 1);
                        return;
                    }
                }

                throw new Error(`No project with id '${projectID}' found`);
            },
        };

        cb(mutator);

        const stateString = JSON.stringify(state);
        localStorage.setItem(ProjectRepositoryState.LOCAL_STORAGE_KEY, stateString);
    }

    public queryAll(): Project[] {
        return this.projects;
    }

    public getActive(): Project | undefined {
        return this.activeProject;
    }

    private static LOCAL_STORAGE_KEY = "project-repository";

}

class LocalStorageProjectRepository implements ProjectRepository {
    private state: ProjectRepositoryState = new ProjectRepositoryState();
    private activeProjectChangedHooks: ProjectEventHook[] = [];

    public createWithRandomID(name: string, description: string): Project {
        const id = crypto.randomUUID();
        const proj: Project = {
            id,
            name,
            description,
        };

        this.saveProject(proj);

        return proj;
    }

    public queryAll(): Project[] {
        return this.state.queryAll();
    }

    public deleteByID(id: string): void {
        this.state.modify((mutator) => mutator.removeProject(id));
    }

    public deleteByName(name: string): void {
        const projects = this.queryAll();

        for (const project of projects) {
            if (project.name === name) {
                this.deleteByID(project.id);
                return;
            }
        }

        throw new Error(`No project with name '${name}' found`);
    }

    public update({ oldName, newName, description }: ProjectUpdateParams ): void {
        const proj = this.queryByName(oldName);
        if (!proj) {
            throw new Error(`Cannot find a project with name '${oldName}'`);
        }

        this.deleteByName(oldName);

        if (newName !== undefined)     proj.name = newName;
        if (description !== undefined) proj.description = description;

        this.saveProject(proj);
    }

    public queryByName(name: string): Project | null {
        const projects = this.queryAll();

        for (const project of projects) {
            if (project.name === name) return project;
        }

        return null;
    }

    public queryByID(id: string): Project | null {
        const projects = this.queryAll();

        for (const project of projects) {
            if (project.id === id) return project;
        }

        return null;
    }

    public setActive(project: Project): void {
        this.state.modify((mutator) => mutator.setActiveProject(project));

        for (const hook of this.activeProjectChangedHooks) hook(project);
    }

    public getActive(): Project | undefined {
        return this.state.getActive();
    }

    public attachEventHook(selector: ProjectEventSelector, hook: ProjectEventHook): void {
        switch (selector.type) {
            case "active-project-changed":
                this.activeProjectChangedHooks.push(hook);
                break;
        }
    }

    private saveProject(proj: Project): void {
        this.state.modify((mutator) => mutator.addProject(proj));
    }
}

export type User = Readonly<{
    id: string;
    firstName: string;
    lastName: string;
}>;

export type UserEventSelector = {
    type: "active-user-changed";
};

export type UserEventHook = (user: User) => void;

interface UserRepository {
    queryByID(id: string): User | null;
    getActive(): User;
    setActiveUser(user: User): void;
    attachEventHook(selector: UserEventSelector, hook: UserEventHook): void;
}

type UserRepositoryMutator = {
    setActiveUser(user: User): void;
    addUser(user: User): void;
};

class UserRepositoryState {
    private users: User[] = [];
    private activeUser: User;

    constructor() {
        const existingState = localStorage.getItem(UserRepositoryState.LOCAL_STORAGE_KEY);

        if (!existingState) {
            this.activeUser = adminUser;
            this.users.push(this.activeUser);

            const thisString = JSON.stringify(this);
            localStorage.setItem(UserRepositoryState.LOCAL_STORAGE_KEY, thisString);
        } else {
            const state = JSON.parse(existingState) as UserRepositoryState;

            this.activeUser = state.activeUser;
            this.users = state.users;
        }
    }

    public modify(cb: (mutator: UserRepositoryMutator) => void): void {
        const repositoryState = this;

        const mutator: UserRepositoryMutator = {
            setActiveUser(user: User): void {
                repositoryState.activeUser = user;
            },

            addUser(user: User): void {
                repositoryState.users.push(user);
            },
        };

        cb(mutator);

        const stateJSONString = JSON.stringify(this);
        localStorage.setItem(UserRepositoryState.LOCAL_STORAGE_KEY, stateJSONString);
    }

    public queryByID(id: string): User | null {
        for (const user of this.users) {
            if (user.id === id) return user;
        }

        return null;
    }

    public getActive(): User {
        return this.activeUser;
    }

    private static LOCAL_STORAGE_KEY: string = "user-repository";
}

class LocalStorageUserRepository implements UserRepository {
    private state: UserRepositoryState;
    private activeUserChangedHooks: UserEventHook[] = [];

    constructor() {
        this.state = new UserRepositoryState();
    }

    public getActive(): User {
        return this.state.getActive();
    }

    public queryByID(id: string) {
        return this.state.queryByID(id);
    }

    public setActiveUser(user: User): void {
        this.state.modify((mutator) => mutator.setActiveUser(user));

        for (const hook of this.activeUserChangedHooks) {
            hook(user);
        }
    }

    public attachEventHook(selector: UserEventSelector, hook: UserEventHook): void {
        switch (selector.type) {
            case "active-user-changed":
                this.activeUserChangedHooks.push(hook);
                break;
        }
    }
}

export type FeaturePriority = "low" | "medium" | "high";

export type FeatureState = "todo" | "in-progress" | "done";

export type Feature = {
    id: string;
    name: string;
    description: string;
    priority: FeaturePriority;
    projectID: string;
    creationDate: string;
    ownerID: string;
    state: FeatureState;
};

export function createNewFeatureRightNow(
    {
        name,
        description,
        priority,
        owner,
        project,
    }: {
        name: string,
        description: string,
        priority: FeaturePriority,
        owner: User,
        project: Project,
    },
): Feature {
    const creationDate = (new Date()).toISOString();

    return {
        id: crypto.randomUUID(),
        ownerID: owner.id,
        projectID: project.id,
        state: "todo",
        name,
        description,
        priority,
        creationDate,
    };
}

export interface FeatureRepository {
    addFeature(feat: Feature): void;
    getProjectFeatures(proj: Project): Feature[];
    updateFeature(feat: Feature): void;
};

type FeatureRepositoryMutator = {
    addFeature(feat: Feature): void;
    updateFeature(feat: Feature): void;
};

class FeatureRepositoryState {
    private static LOCAL_STORAGE_KEY = "feature-repository";

    private features: Feature[] = [];

    public constructor() {
        const existingState = localStorage.getItem(FeatureRepositoryState.LOCAL_STORAGE_KEY);

        if (!existingState) {
            const thisString = JSON.stringify(this);
            localStorage.setItem(FeatureRepositoryState.LOCAL_STORAGE_KEY, thisString);
        } else {
            const state = JSON.parse(existingState) as FeatureRepositoryState;
            this.features = state.features;
        }
    }

    public modify(cb: (mutator: FeatureRepositoryMutator) => void): void {
        const state = this;

        const mutator: FeatureRepositoryMutator = {
            addFeature(feat: Feature): void {
                state.features.push(feat);
            },
            updateFeature(feat: Feature): void {
                feat;
            },
        };

        cb(mutator);

        const stateString = JSON.stringify(state);
        localStorage.setItem(FeatureRepositoryState.LOCAL_STORAGE_KEY, stateString);
    }

    public getProjectFeatures(proj: Project): Feature[] {
        return this.features.filter((feat) => feat.projectID === proj.id);
    }
}

class LocalStorageFeatureRepository implements FeatureRepository {
    private state: FeatureRepositoryState = new FeatureRepositoryState();

    public addFeature(feat: Feature): void {
        this.state.modify((mutator) => mutator.addFeature(feat));
    }

    public getProjectFeatures(proj: Project): Feature[] {
        return this.state.getProjectFeatures(proj);
    }

    public updateFeature(feature: Feature): void {
        this.state.modify((mutator) => mutator.updateFeature(feature));
    }
};

const adminUser: User = {
    id: "admin-id",
    firstName: "admin",
    lastName: "adminenko",
};

const globalProjectRepository: ProjectRepository = new LocalStorageProjectRepository();
const globalUserRepository: UserRepository = new LocalStorageUserRepository();
const globalFeatureRepository: FeatureRepository = new LocalStorageFeatureRepository();

export {
    globalProjectRepository,
    globalUserRepository,
    globalFeatureRepository,
};
