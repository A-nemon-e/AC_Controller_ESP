import {
    WebSocketGateway,
    WebSocketServer,
    OnGatewayConnection,
    OnGatewayDisconnect,
} from '@nestjs/websockets';
import { Server, Socket } from 'socket.io';
import { OnEvent } from '@nestjs/event-emitter';
import { Logger } from '@nestjs/common';

@WebSocketGateway({ cors: true, namespace: '/' })
export class DevicesGateway implements OnGatewayConnection, OnGatewayDisconnect {
    @WebSocketServer()
    server: Server;

    private readonly logger = new Logger(DevicesGateway.name);

    handleConnection(client: Socket) {
        this.logger.log(`Client connected: ${client.id}`);
        // TODO: 验证JWT token
    }

    handleDisconnect(client: Socket) {
        this.logger.log(`Client disconnected: ${client.id}`);
    }

    @OnEvent('device.status')
    handleDeviceStatus(payload: { deviceId: number; userId: number; data: any }) {
        // 推送给订阅该设备的客户端
        this.server.emit('device_status', {
            deviceId: payload.deviceId,
            ...payload.data,
        });
        this.logger.debug(`Pushed device_status for device ${payload.deviceId}`);
    }

    @OnEvent('device.ghost')
    handleGhostEvent(payload: { userId: number; deviceId: number; deviceName?: string }) {
        // 推送Ghost警告
        this.server.emit('ghost_detected', {
            deviceId: payload.deviceId,
            deviceName: payload.deviceName,
            timestamp: new Date(),
        });
        this.logger.warn(`Pushed ghost_detected for device ${payload.deviceId}`);
    }

    @OnEvent('device.log')
    handleNewLog(payload: { deviceId: number; log: any }) {
        this.server.emit('new_log', {
            deviceId: payload.deviceId,
            log: payload.log,
        });
        this.logger.debug(`Pushed new_log for device ${payload.deviceId}`);
    }

    @OnEvent('routine.triggered')
    handleRoutineTriggered(payload: { routineId: number; routineName: string }) {
        this.server.emit('routine_triggered', payload);
        this.logger.log(`Pushed routine_triggered: ${payload.routineName}`);
    }
}
