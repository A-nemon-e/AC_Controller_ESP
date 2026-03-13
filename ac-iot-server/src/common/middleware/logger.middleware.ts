import { Injectable, NestMiddleware, Logger } from '@nestjs/common';
import { Request, Response, NextFunction } from 'express';

/**
 * 增强的请求日志中间件
 * 记录每个请求的详细信息，包括：
 * - 请求方法、URL、IP
 * - 响应状态码、响应时间、内容长度
 * - 用户代理信息
 * - 根据状态码区分日志级别（error/warn/log）
 */
@Injectable()
export class LoggerMiddleware implements NestMiddleware {
  private logger = new Logger('HTTP');

  use(req: Request, res: Response, next: NextFunction) {
    const startTime = Date.now();
    const { ip, method, originalUrl } = req;
    const userAgent = req.get('user-agent') || '';

    // 记录请求开始
    this.logger.log(
      `>> [REQUEST] ${method} ${originalUrl} - IP: ${ip} - UA: ${userAgent}`,
    );

    // 响应完成后记录
    res.on('finish', () => {
      const duration = Date.now() - startTime;
      const { statusCode } = res;
      const contentLength = res.get('content-length') || 0;

      const logMessage = `${method} ${originalUrl} ${statusCode} ${contentLength}b ${duration}ms - ${userAgent} ${ip}`;

      // 根据状态码选择日志级别
      if (statusCode >= 500) {
        this.logger.error(`[RESPONSE] ${logMessage}`);
      } else if (statusCode >= 400) {
        this.logger.warn(`[RESPONSE] ${logMessage}`);
      } else {
        this.logger.log(`[RESPONSE] ${logMessage}`);
      }
    });

    next();
  }
}
