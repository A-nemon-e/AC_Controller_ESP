export const PASSWORD_POLICY = {
  minLength: 8,
  maxLength: 32,
  requireUppercase: true,
  requireLowercase: true,
  requireNumbers: true,
  requireSpecialChars: false,
  maxAge: 90,
  historyCount: 3,
};

export const LOGIN_SECURITY = {
  maxFailedAttempts: 5,
  lockoutDuration: 15,
  requireCaptchaAfter: 3,
};

export const ERROR_CODES = {
  DEVICE_ALREADY_BOUND: 'DEVICE_001',
  DEVICE_BOUND_TO_OTHER: 'DEVICE_002',
  DEVICE_NOT_REBINDABLE: 'DEVICE_003',
  BINDING_FAILED: 'DEVICE_004',
  AGREEMENT_REQUIRED: 'AGREEMENT_001',
  AGREEMENT_OUTDATED: 'AGREEMENT_002',
  PASSWORD_INCORRECT: 'PASSWORD_001',
  PASSWORD_POLICY_VIOLATION: 'PASSWORD_002',
  PASSWORD_REUSED: 'PASSWORD_003',
  PASSWORD_EXPIRED: 'PASSWORD_004',
  ADMIN_REQUIRED: 'ADMIN_001',
  PERMISSION_DENIED: 'ADMIN_002',
};
