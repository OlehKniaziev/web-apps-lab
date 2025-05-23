const BACKEND_URL = "http://localhost:5959";

export type Project = {
    Id: string;
    Name: string;
    Description: string;
};

export type ProjectUpdateParams = {
    Id: string;
    Name?: string | undefined;
    Description?: string | undefined;
};

type ProjectEventSelector = {
    type: "active-project-changed";
};

type ProjectEventHook = (proj: Project) => void;

interface ProjectRepository {
    createWithRandomID(name: string, description: string): Project;
    queryAll(): Promise<Project[]>;
    queryByName(name: string): Promise<Project | null>;
    queryByID(id: string): Promise<Project | null>;
    deleteByName(name: string): void;
    deleteByID(id: string): void;
    update(params: ProjectUpdateParams): void;
    setActive(project: Project): void;
    getActive(): Project | undefined;
    attachEventHook(selector: ProjectEventSelector, hook: ProjectEventHook): void;
}

class LocalStorageProjectRepository implements ProjectRepository {
    private activeProject?: Project;
    private activeProjectChangedHooks: ProjectEventHook[] = [];

    public createWithRandomID(name: string, description: string): Project {
        const id = crypto.randomUUID();
        const proj: Project = {
            Id: id,
            Name: name,
            Description: description,
        };

        this.saveProject(proj);

        return proj;
    }

    public async queryAll(): Promise<Project[]> {
        const resp = await fetch(`${BACKEND_URL}/get-all-projects`);
        const json = await resp.json();
        // TODO(oleh): Validation.
        return json as Project[];
    }

    public deleteByID(id: string): void {
        fetch(`${BACKEND_URL}/delete-project`, {
            method: "POST",
            body: id,
        });
    }

    public async deleteByName(name: string): Promise<void> {
        const projects = await this.queryAll();

        for (const project of projects) {
            if (project.Name === name) {
                this.deleteByID(project.Id);
                return;
            }
        }

        throw new Error(`No project with name '${name}' found`);
    }

    public update(params: ProjectUpdateParams): void {
        fetch(`${BACKEND_URL}/update-project`, {
            method: "POST",
            body: JSON.stringify(params),
        });
    }

    public async queryByName(name: string): Promise<Project | null> {
        const projects = await this.queryAll();

        for (const project of projects) {
            if (project.Name === name) return project;
        }

        return null;
    }

    public async queryByID(id: string): Promise<Project | null> {
        const projects = await this.queryAll();

        for (const project of projects) {
            if (project.Id === id) return project;
        }

        return null;
    }

    public setActive(project: Project): void {
        this.activeProject = project;
        for (const hook of this.activeProjectChangedHooks) hook(project);
    }

    public getActive(): Project | undefined {
        return this.activeProject;
    }

    public attachEventHook(selector: ProjectEventSelector, hook: ProjectEventHook): void {
        switch (selector.type) {
            case "active-project-changed":
                this.activeProjectChangedHooks.push(hook);
                break;
        }
    }

    private saveProject(proj: Project): void {
        fetch(`${BACKEND_URL}/insert-project`, {
            method: "POST",
            body: JSON.stringify(proj),
        });
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
        projectID: project.Id,
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
        return this.features.filter((feat) => feat.projectID === proj.Id);
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
