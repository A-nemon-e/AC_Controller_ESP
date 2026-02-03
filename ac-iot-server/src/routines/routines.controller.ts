import { Controller, Get, Post, Put, Delete, Body, Param, UseGuards, Request, ParseIntPipe } from '@nestjs/common';
import { RoutinesService } from './routines.service';
import { CreateRoutineDto } from './dto/create-routine.dto';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';

@UseGuards(JwtAuthGuard)
@Controller('routines')
export class RoutinesController {
    constructor(private readonly routinesService: RoutinesService) { }

    @Post()
    create(@Request() req: any, @Body() createRoutineDto: CreateRoutineDto) {
        return this.routinesService.create(req.user.userId, createRoutineDto);
    }

    @Get()
    findAll(@Request() req: any) {
        return this.routinesService.findAll(req.user.userId);
    }

    @Get(':id')
    findOne(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.routinesService.findOne(req.user.userId, id);
    }

    @Put(':id')
    update(@Request() req: any, @Param('id', ParseIntPipe) id: number, @Body() updateDto: CreateRoutineDto) {
        return this.routinesService.update(req.user.userId, id, updateDto);
    }

    @Delete(':id')
    remove(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.routinesService.remove(req.user.userId, id);
    }
}
