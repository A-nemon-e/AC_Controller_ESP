import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { RoutinesService } from './routines.service';
import { RoutinesController } from './routines.controller';
import { Routine } from './routine.entity';
import { DevicesModule } from '../devices/devices.module';

@Module({
    imports: [
        TypeOrmModule.forFeature([Routine]),
        DevicesModule,
    ],
    controllers: [RoutinesController],
    providers: [RoutinesService],
    exports: [RoutinesService],
})
export class RoutinesModule { }
