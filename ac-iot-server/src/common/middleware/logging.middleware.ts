import { Injectable, NestMiddleware } from '@nestjs/common';
import { Request, Response, NextFunction } from 'express';

/**
 * 请求日志中间件
 * 记录每个请求的详细信息，包括：
 * - 请求方法、URL、IP
 * - 响应状态码、响应时间
 * - 用户代理信息
 */
@Injectable()
export class LoggingMiddleware implements NestMiddleware {
  private readonly logger = console;

  use(req: Request, res: Response, next: NextFunction) {
    const startTime = Date.now();
    const { method, originalUrl, ip } = req;
    const userAgent = req.get('user-agent') || 'unknown';

    // 记录请求开始
    this.logger.log(
      `[REQUEST] ${method} ${originalUrl} - IP: ${ip} - UA: ${userAgent}`,
    );

    // 响应完成后记录
    res.on('finish', () => {
      const duration = Date.now() - startTime;
      const statusCode = res.statusCode;
      const statusMessage = res.statusMessage || '';

      // 根据状态码选择日志级别
      if (statusCode >= 500) {
        this.logger.error(
          `[RESPONSE] ${method} ${originalUrl} - ${statusCode} ${statusMessage} - ${duration}ms`,
        );
      } else if (statusCode >= 400) {
        this.logger.warn(
          `[RESPONSE] ${method} ${originalUrl} - ${statusCode} ${statusMessage} - ${duration}ms`,
        );
      } else {
        this.logger.log(
          `[RESPONSE] ${method} ${originalUrl} - ${statusCode} - ${duration}ms`,
        );
      }
    });

    next();
  }
}
