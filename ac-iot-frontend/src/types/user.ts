// 用户类型
export interface User {
    id: number
    username: string
    email?: string
}

export interface LoginRequest {
    username: string
    password: string
}

export interface LoginResponse {
    access_token: string
}
