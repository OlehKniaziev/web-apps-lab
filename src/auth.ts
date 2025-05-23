import { authGetFirstNameInput, authGetLastNameInput, authGetRoleInput } from "./dom.ts";

export async function googleLetMeIn(data: any) {
    const { given_name: firstName, family_name: lastName } = parseJwt(data.credential);
    const firstNameInput = authGetFirstNameInput();
    const lastNameInput = authGetLastNameInput();
    const roleInput = authGetRoleInput();

    firstNameInput.value = firstName;
    lastNameInput.value = lastName;
    roleInput.value = "guest";
}

function parseJwt(token: string) {
    const base64Url = token.split('.')[1]
    const base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/')
    const jsonPayload = decodeURIComponent(
        window
            .atob(base64)
            .split('')
            .map(c => '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2))
            .join('')
    )

    return JSON.parse(jsonPayload)
}
