import {
  Injectable,
  CanActivate,
  ExecutionContext,
  ForbiddenException,
  UnauthorizedException,
} from '@nestjs/common';
import { UserRole } from '../users/user.entity';

@Injectable()
export class AdminGuard implements CanActivate {
  canActivate(context: ExecutionContext): boolean {
    const request = context.switchToHttp().getRequest();
    const user = request.user;

    if (!user) {
      throw new UnauthorizedException();
    }

    if (user.role === UserRole.USER) {
      throw new ForbiddenException('Admin access required');
    }

    return true;
  }
}

export const PERMISSIONS = {
  'user.view': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
  'user.create': [UserRole.SUPER_ADMIN],
  'user.edit': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
  'user.delete': [UserRole.SUPER_ADMIN],
  'device.view_all': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
  'system.stats': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
};
