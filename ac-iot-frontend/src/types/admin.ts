export interface AdminStats {
  totalUsers: number
  activeUsersToday: number
  totalDevices: number
  onlineDevices: number
  newUsersToday: number
  newDevicesToday: number
  totalCommands?: number
  activeToday?: number
}

export interface UserWithDevices {
  id: number
  username: string
  email: string
  role: string
  isActive: boolean
  createdAt: string
  lastLoginAt?: string
  deviceCount: number
  lastDeviceName?: string
}

export interface DeviceWithUser {
  id: number
  uuid: string
  name: string
  isOnline: boolean
  brand?: string
  model?: number
  firmwareVersion?: string
  userId?: number
  username?: string
  ownerName?: string
  mac?: string
  ip?: string
  createdAt: string
  lastSeenAt?: string
}

export interface OtaPackage {
  id: string
  version: string
  description?: string
  firmwareUrl: string
  fileSize: number
  targetDevices: string[]
  successCount?: number
  status: 'draft' | 'publishing' | 'published' | 'archived'
  createdAt?: string
  releaseDate?: string
  changelog?: string
  isMandatory?: boolean
  rolloutPercentage?: number
}

export interface OtaUpdate {
  id: string
  packageId: string
  deviceId: number
  deviceUuid: string
  status: 'pending' | 'downloading' | 'installing' | 'completed' | 'failed' | 'rolled_back'
  progress: number
}

export interface SystemLog {
  id: string
  level: 'info' | 'warning' | 'error'
  message: string
  source?: string
  component?: string
  timestamp?: string
  createdAt?: string
}
