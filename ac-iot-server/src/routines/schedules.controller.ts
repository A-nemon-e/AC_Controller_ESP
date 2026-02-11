import { Controller, Get, Post, Put, Delete, Body, Param, UseGuards, Request, ParseIntPipe, Logger } from '@nestjs/common';
import { RoutinesService } from './routines.service';
import { CreateScheduleDto } from './dto/create-schedule.dto';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { Routine } from './routine.entity';

@UseGuards(JwtAuthGuard)
@Controller('schedules')
export class SchedulesController {
    private readonly logger = new Logger(SchedulesController.name);

    constructor(private readonly routinesService: RoutinesService) { }

    @Get()
    async findAll(@Request() req: any) {
        const routines = await this.routinesService.findAll(req.user.userId);
        // Filter routines that are valid schedules (have cron, no triggers)
        // Note: We assume schedules have no sensor triggers. 
        // If a routine has triggers, it's not a simple time schedule in this context.
        const schedules = routines
            .filter(r => r.cron && (!r.triggers || !Array.isArray(r.triggers) || r.triggers.length === 0))
            .map(r => this.mapRoutineToSchedule(r));
        return schedules;
    }

    @Post()
    async create(@Request() req: any, @Body() createDto: CreateScheduleDto) {
        const routineData = this.mapScheduleToRoutine(createDto);
        const routine = await this.routinesService.create(req.user.userId, routineData as any);
        return this.mapRoutineToSchedule(routine);
    }

    @Put(':id')
    async update(@Request() req: any, @Param('id', ParseIntPipe) id: number, @Body() updateDto: Partial<CreateScheduleDto>) {
        // First get existing to merge (since we need full schedule to rebuild cron if time changed)
        const existingRoutine = await this.routinesService.findOne(req.user.userId, id);
        const existingSchedule = this.mapRoutineToSchedule(existingRoutine);

        // Merge updates
        const mergedSchedule = { ...existingSchedule, ...updateDto };

        // Rebuild routine data
        const routineData = this.mapScheduleToRoutine(mergedSchedule as CreateScheduleDto);

        // Update
        const updated = await this.routinesService.update(req.user.userId, id, routineData as any);
        return this.mapRoutineToSchedule(updated);
    }

    @Delete(':id')
    async remove(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.routinesService.remove(req.user.userId, id);
    }

    // --- Helpers ---

    private mapRoutineToSchedule(routine: Routine) {
        const { hour, minute, repeat, weekdays } = this.parseCron(routine.cron);

        // routine.actions is array, schedule.action is single object (we take the first one)
        const action = (routine.actions && routine.actions.length > 0) ? routine.actions[0].cmd : {};
        const deviceId = (routine.actions && routine.actions.length > 0) ? routine.actions[0].deviceId : 0;

        return {
            id: routine.id,
            name: routine.name,
            deviceId: deviceId,
            enabled: routine.enabled,
            hour,
            minute,
            repeat,
            weekdays, // optional
            action,
            lastRun: routine.lastRun ? routine.lastRun.toISOString() : null,
            userId: routine.userId
        };
    }

    private mapScheduleToRoutine(dto: CreateScheduleDto) {
        const cron = this.buildCron(dto.hour, dto.minute, dto.repeat, dto.weekdays);

        // Wrap action in array with deviceId
        const actions = [{
            deviceId: dto.deviceId,
            cmd: dto.action
        }];

        return {
            name: dto.name,
            enabled: dto.enabled !== undefined ? dto.enabled : true,
            cron: cron,
            triggers: [], // Empty for pure schedule
            actions: actions
        };
    }

    private buildCron(hour: number, minute: number, repeat: string, weekdays?: number[]): string {
        // Format: "minute hour * * dow"
        const min = minute;
        const hr = hour;

        let dow = '*';
        if (repeat === 'daily') dow = '*';
        else if (repeat === 'weekdays') dow = '1-5';
        else if (repeat === 'weekends') dow = '0,6';
        else if (repeat === 'custom' && weekdays && weekdays.length > 0) {
            dow = weekdays.join(',');
        }

        // Use 6 fields for node-cron (second minute hour day month dow) 
        // OR 5 fields. routines.service.ts example "0 22 * * *" is 5 fields.
        // Let's use 5 fields standard. "min hour day month dow"
        return `${min} ${hr} * * ${dow}`;
    }

    private parseCron(cron: string) {
        if (!cron) return { hour: 0, minute: 0, repeat: 'daily', weekdays: [] };

        const parts = cron.trim().split(' ');
        // Support 5 or 6 parts. If 6, first is second.
        let minIndex = 0;
        let hourIndex = 1;
        let dowIndex = 4;

        if (parts.length === 6) {
            minIndex = 1;
            hourIndex = 2;
            dowIndex = 5;
        }

        const minute = parseInt(parts[minIndex], 10) || 0;
        const hour = parseInt(parts[hourIndex], 10) || 0;
        const dow = parts[dowIndex];

        let repeat = 'custom';
        let weekdays: number[] = [];

        if (dow === '*' || dow === '0-6' || dow === '0-7') {
            repeat = 'daily';
        } else if (dow === '1-5') {
            repeat = 'weekdays';
        } else if (dow === '0,6' || dow === '6,0') {
            repeat = 'weekends';
        } else {
            // custom
            repeat = 'custom';
            // Simple parse of comma separated. 
            // Note: Does not handle complex cron like '1-3,5' perfectly but sufficient for our generator
            weekdays = dow.split(',').map(d => parseInt(d, 10)).filter(n => !isNaN(n));
        }

        return { hour, minute, repeat, weekdays };
    }
}
