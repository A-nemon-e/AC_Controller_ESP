import { Injectable, NestMiddleware, Logger } from '@nestjs/common';
import { Request, Response, NextFunction } from 'express';

@Injectable()
export class LoggerMiddleware implements NestMiddleware {
    private logger = new Logger('HTTP');

    use(req: Request, res: Response, next: NextFunction) {
        const { ip, method, originalUrl } = req;
        const userAgent = req.get('user-agent') || '';

        res.on('finish', () => {
            const { statusCode } = res;
            const contentLength = res.get('content-length');

            this.logger.log(
                `${method} ${originalUrl} ${statusCode} ${contentLength} - ${userAgent} ${ip}`
            );
        });

        // 也打印请求进入时的日志，以防请求卡住或未完成
        this.logger.log(`>> Request Incoming: ${method} ${originalUrl}`);

        next();
    }
}
