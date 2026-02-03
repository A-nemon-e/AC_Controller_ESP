import { Injectable, NotFoundException, ForbiddenException, OnModuleInit, Logger } from '@nestjs/common';
import { SchedulerRegistry } from '@nestjs/schedule';
import { CronJob } from 'cron';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Routine } from './routine.entity';
import { CreateRoutineDto } from './dto/create-routine.dto';

import { DevicesService } from '../devices/devices.service';

@Injectable()
export class RoutinesService implements OnModuleInit {
    private readonly logger = new Logger(RoutinesService.name);

    constructor(
        @InjectRepository(Routine)
        private routinesRepository: Repository<Routine>,
        private schedulerRegistry: SchedulerRegistry,
        private devicesService: DevicesService,
    ) { }

    async onModuleInit() {
        this.logger.log('Loading saved routines...');
        const routines = await this.routinesRepository.find();
        routines.forEach(routine => {
            if (routine.enabled && routine.cron) {
                this.addCronJob(routine);
            }
        });
    }

    private addCronJob(routine: Routine) {
        const jobName = `routine-${routine.id}`;

        try {
            // Note: CronJob type might mismatch between 'cron' package and @nestjs/schedule internal dependency
            const job = new CronJob(routine.cron, async () => {
                this.logger.log(`Executing Cron Routine: ${routine.name}`);
                await this.executeRoutineActions(routine);
            });

            // Cast to any to avoid strict version mismatch of CronJob class
            this.schedulerRegistry.addCronJob(jobName, job as any);
            job.start();
            this.logger.log(`Added Cron Job: ${jobName} -> ${routine.cron}`);
        } catch (e) {
            this.logger.error(`Failed to add Cron Job ${jobName}: ${e.message}`);
        }
    }

    private deleteCronJob(routineId: number) {
        const jobName = `routine-${routineId}`;
        try {
            this.schedulerRegistry.deleteCronJob(jobName);
            this.logger.log(`Deleted Cron Job: ${jobName}`);
        } catch (e) {
            // Ignore if not found
        }
    }

    /**
     * Evaluate event-based rules for a specific device based on new sensor data
     */
    async evaluateRules(deviceId: number, sensorData: { temp?: number, hum?: number }) {
        if (sensorData.temp === undefined && sensorData.hum === undefined) return;

        // 1. Find the device owner
        const device = await this.devicesService.findOneById(deviceId);
        if (!device) return;

        // 2. Find enabled routines for this user
        // Optimization: In real world, cache this or use efficient DB query
        const routines = await this.routinesRepository.find({ where: { userId: device.userId, enabled: true } });

        for (const routine of routines) {
            if (!routine.triggers || !Array.isArray(routine.triggers)) continue;

            // Iterate Triggers (AND logic)
            const triggers = routine.triggers as any[];
            const allMatch = triggers.every(trigger => {
                // { type: 'temp', op: '>', val: 28 }
                let currentVal: number | undefined;

                if (trigger.type === 'temp') currentVal = sensorData.temp;
                else if (trigger.type === 'hum') currentVal = sensorData.hum;
                else return true; // Ignore triggers not related to sensor

                if (currentVal === undefined) return false;

                switch (trigger.op) {
                    case '>': return currentVal > trigger.val;
                    case '<': return currentVal < trigger.val;
                    case '>=': return currentVal >= trigger.val;
                    case '<=': return currentVal <= trigger.val;
                    case '=': return currentVal == trigger.val;
                    default: return false;
                }
            });

            if (allMatch && triggers.length > 0) {
                this.logger.log(`Routine "${routine.name}" triggered by Sensor Rule for Device ${deviceId}`);
                await this.executeRoutineActions(routine, deviceId);
            }
        }
    }

    private async executeRoutineActions(routine: Routine, triggeringDeviceId?: number) {
        if (!routine.actions || !Array.isArray(routine.actions)) return;

        for (const action of routine.actions as any[]) {
            // Action: { deviceId?: number, cmd: {...} }
            const targetDevId = action.deviceId || triggeringDeviceId;

            if (!targetDevId) {
                this.logger.warn(`Routine "${routine.name}" action missing target device ID`);
                continue;
            }

            try {
                await this.devicesService.sendCommand(routine.userId, targetDevId, action.cmd);
            } catch (e) {
                this.logger.error(`Failed to execute routine action: ${e.message}`);
            }
        }
    }

    async create(userId: number, createRoutineDto: CreateRoutineDto): Promise<Routine> {
        const routine = this.routinesRepository.create({
            ...createRoutineDto,
            userId,
        });
        const saved = await this.routinesRepository.save(routine);
        if (saved.enabled && saved.cron) {
            this.addCronJob(saved);
        }
        return saved;
    }

    async findAll(userId: number): Promise<Routine[]> {
        return this.routinesRepository.find({ where: { userId } });
    }

    async findOne(userId: number, id: number): Promise<Routine> {
        const routine = await this.routinesRepository.findOne({ where: { id } });
        if (!routine) throw new NotFoundException('Routine not found');
        if (routine.userId !== userId) throw new ForbiddenException('Access denied');
        return routine;
    }

    async update(userId: number, id: number, updateData: Partial<CreateRoutineDto>): Promise<Routine> {
        const routine = await this.findOne(userId, id);
        this.deleteCronJob(id); // Remove old job

        Object.assign(routine, updateData);
        const saved = await this.routinesRepository.save(routine);

        if (saved.enabled && saved.cron) {
            this.addCronJob(saved);
        }
        return saved;
    }

    async remove(userId: number, id: number): Promise<void> {
        const routine = await this.findOne(userId, id);
        this.deleteCronJob(id);
        await this.routinesRepository.remove(routine);
    }
}
