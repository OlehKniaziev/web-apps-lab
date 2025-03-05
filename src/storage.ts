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

interface ProjectRepository {
    createWithRandomID(name: string, description: string): Project;
    queryAll(): Project[];
    queryByName(name: string): Project | null;
    deleteByName(name: string): void;
    update(params: ProjectUpdateParams): void;
}

class LocalStorageProjectRepository implements ProjectRepository {
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
        const projects = [];

        for (const projectName of this.projectNames()) {
            const projJSON = localStorage.getItem(projectName)!;
            const proj = JSON.parse(projJSON);
            projects.push(proj);
        }

        return projects;
    }

    public deleteByName(name: string): void {
        localStorage.removeItem(name);
    }

    public update({ oldName, newName, description }: ProjectUpdateParams ): void {
        const proj = this.queryByName(oldName);
        if (!proj) {
            throw new Error(`Cannot find a project with name '${oldName}'`);
        }

        if (newName !== undefined)     proj.name = newName;
        if (description !== undefined) proj.description = description;

        this.deleteByName(oldName);

        const projString = JSON.stringify(proj);
        localStorage.setItem(proj.name, projString);
    }

    public queryByName(name: string): Project | null {
        const projectString = localStorage.getItem(name);
        if (projectString !== null) return JSON.parse(projectString);
        return null;
    }

    private saveProject(proj: Project): void {
        const serialized = JSON.stringify(proj);

        if (localStorage.getItem(proj.name) !== null) {
            throw new Error(`Cannot create a project: name '${proj.name}' is already taken`);
        }

        localStorage.setItem(proj.name, serialized);
    }

    private *projectNames(): Generator<string> {
        for (let i = 0; i < localStorage.length; i++) {
            yield localStorage.key(i)!;
        }
    }
}

const globalProjectRepository: ProjectRepository = new LocalStorageProjectRepository();

export {
    globalProjectRepository,
};
