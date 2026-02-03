import { defineStore } from 'pinia'
import { ref } from 'vue'
import { schedulesApi, routinesApi } from '@/api/automation'
import type { Schedule, Routine } from '@/types/automation'

export const useAutomationStore = defineStore('automation', () => {
    const schedules = ref<Schedule[]>([])
    const routines = ref<Routine[]>([])
    const loading = ref(false)

    const fetchSchedules = async () => {
        loading.value = true
        try {
            schedules.value = await schedulesApi.getAll()
        } catch (error) {
            console.error('Failed to fetch schedules:', error)
        } finally {
            loading.value = false
        }
    }

    const fetchRoutines = async () => {
        loading.value = true
        try {
            routines.value = await routinesApi.getAll()
        } catch (error) {
            console.error('Failed to fetch routines:', error)
        } finally {
            loading.value = false
        }
    }

    const addSchedule = (schedule: Schedule) => {
        schedules.value.push(schedule)
    }

    const updateSchedule = (id: number, data: Partial<Schedule>) => {
        const index = schedules.value.findIndex((s) => s.id === id)
        if (index !== -1) {
            schedules.value[index] = { ...schedules.value[index], ...data }
        }
    }

    const removeSchedule = (id: number) => {
        const index = schedules.value.findIndex((s) => s.id === id)
        if (index !== -1) {
            schedules.value.splice(index, 1)
        }
    }

    const addRoutine = (routine: Routine) => {
        routines.value.push(routine)
    }

    const updateRoutine = (id: number, data: Partial<Routine>) => {
        const index = routines.value.findIndex((r) => r.id === id)
        if (index !== -1) {
            routines.value[index] = { ...routines.value[index], ...data }
        }
    }

    const removeRoutine = (id: number) => {
        const index = routines.value.findIndex((r) => r.id === id)
        if (index !== -1) {
            routines.value.splice(index, 1)
        }
    }

    return {
        schedules,
        routines,
        loading,
        fetchSchedules,
        fetchRoutines,
        addSchedule,
        updateSchedule,
        removeSchedule,
        addRoutine,
        updateRoutine,
        removeRoutine,
    }
})
