import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { CardConfig, DisplaySettings, WeatherOverlay } from '@/types/display'
import { displayApi } from '@/api/display'

export const useDisplayStore = defineStore('display', () => {
    // State
    const settings = ref<DisplaySettings>({
        brightness: 80,
        autoSwitch: true,
        switchInterval: 10,
        transition: 'fade',
        transitionSpeed: 500,
        nightMode: true,
        nightModeStart: '22:00',
        nightModeEnd: '07:00',
        nightModeBrightness: 30,
    })

    const cards = ref<CardConfig[]>([
        {
            id: '1',
            name: '时钟卡片',
            enabled: true,
            duration: 10,
            background: {
                type: 'gradient',
                brightness: 128,
                gradientColors: ['#667eea', '#764ba2'],
            },
            foregrounds: [
                {
                    type: 'clock',
                    position: 'middle-center',
                    font: 'large',
                    color: '#ffffff',
                    brightness: 255,
                    enabled: true,
                },
            ],
        },
        {
            id: '2',
            name: '天气卡片',
            enabled: true,
            duration: 10,
            background: {
                type: 'solid',
                brightness: 128,
                color: '#1989fa',
            },
            foregrounds: [
                {
                    type: 'temperature',
                    position: 'middle-center',
                    font: 'medium',
                    color: '#ffffff',
                    brightness: 255,
                    enabled: true,
                    suffix: '°C',
                },
                {
                    type: 'weather',
                    position: 'bottom-center',
                    font: 'small',
                    color: '#ffffff',
                    brightness: 255,
                    enabled: true,
                },
            ],
        },
    ])

    const weatherOverlay = ref<WeatherOverlay>({
        enabled: true,
        showIcon: true,
        showTemperature: true,
        position: 'top-right',
    })

    const loading = ref(false)
    const currentCardId = ref<string>('1')
    const currentDeviceId = ref<string>('')

    // Getters
    const enabledCards = computed(() => cards.value.filter(card => card.enabled))
    const currentCard = computed(() => cards.value.find(card => card.id === currentCardId.value))
    const currentCardIndex = computed(() => enabledCards.value.findIndex(card => card.id === currentCardId.value))

    // Actions
    const updateSettings = (newSettings: Partial<DisplaySettings>) => {
        settings.value = { ...settings.value, ...newSettings }
    }

    const addCard = (card: Omit<CardConfig, 'id'>) => {
        const id = String(Date.now())
        cards.value.push({ ...card, id })
        return id
    }

    const updateCard = (id: string, card: Partial<CardConfig>) => {
        const index = cards.value.findIndex(c => c.id === id)
        if (index !== -1) {
            cards.value[index] = { ...cards.value[index], ...card }
        }
    }

    const deleteCard = (id: string) => {
        const index = cards.value.findIndex(c => c.id === id)
        if (index !== -1) {
            cards.value.splice(index, 1)
        }
    }

    const reorderCards = (newOrder: string[]) => {
        const orderedCards: CardConfig[] = []
        newOrder.forEach(id => {
            const card = cards.value.find(c => c.id === id)
            if (card) orderedCards.push(card)
        })
        cards.value = orderedCards
    }

    const setCurrentCard = (id: string) => {
        currentCardId.value = id
    }

    const nextCard = () => {
        if (!settings.value.autoSwitch) return
        const enabled = enabledCards.value
        if (enabled.length === 0) return
        const currentIndex = enabled.findIndex(card => card.id === currentCardId.value)
        const nextIndex = (currentIndex + 1) % enabled.length
        currentCardId.value = enabled[nextIndex].id
    }

    const updateWeatherOverlay = (overlay: Partial<WeatherOverlay>) => {
        weatherOverlay.value = { ...weatherOverlay.value, ...overlay }
    }

    const fetchSettings = async (deviceId?: string) => {
        loading.value = true
        try {
            const targetDeviceId = deviceId || currentDeviceId.value
            if (!targetDeviceId) {
                console.warn('No device ID provided for fetchSettings')
                return
            }
            
            const response = await displayApi.getSettings(targetDeviceId)
            if (response.data) {
                settings.value = response.data.settings || settings.value
                cards.value = response.data.cards || cards.value
            }
        } catch (error) {
            console.error('Failed to fetch display settings:', error)
        } finally {
            loading.value = false
        }
    }

    const saveSettings = async (deviceId?: string) => {
        loading.value = true
        try {
            const targetDeviceId = deviceId || currentDeviceId.value
            if (!targetDeviceId) {
                console.warn('No device ID provided for saveSettings')
                return
            }
            
            await displayApi.saveSettings(targetDeviceId, settings.value)
            await displayApi.saveCards(targetDeviceId, cards.value)
        } catch (error) {
            console.error('Failed to save display settings:', error)
            throw error
        } finally {
            loading.value = false
        }
    }

    const setDeviceId = (deviceId: string) => {
        currentDeviceId.value = deviceId
    }

    return {
        settings,
        cards,
        weatherOverlay,
        loading,
        currentCardId,
        currentDeviceId,
        enabledCards,
        currentCard,
        currentCardIndex,
        updateSettings,
        addCard,
        updateCard,
        deleteCard,
        reorderCards,
        setCurrentCard,
        nextCard,
        updateWeatherOverlay,
        fetchSettings,
        saveSettings,
        setDeviceId,
    }
})
