import {defineStore} from 'pinia'
import {ref} from 'vue'

export const useAppStore = defineStore('app', ()=>{

    // STATES 

    // ACTIONS
    const getReserveInRange = async (start, end) => {
        const controller = new AbortController()
        const signal = controller.signal
        const id = setTimeout(() => { controller.abort() }, 60000)
        const URL = `/api/reserve/${start}/${end}`
        try {
            const response = await fetch(URL, { method: 'GET', signal: signal })
            if (response.ok) {
                const data = await response.json()
                let keys = Object.keys(data)
                if (keys.includes("status")) {
                    if (data["status"] == "found") {
                        return data["data"]
                    }
                    if (data["status"] == "failed") {
                        console.log("getReserveInRange returned no data")
                    }
                }
            }
        } catch (err) {
            console.error('getReserveInRange error: ', err.message)
        }
        return []
    }

    const getAverage = async (start, end) => {
        const controller = new AbortController()
        const signal = controller.signal
        const id = setTimeout(() => { controller.abort() }, 60000)
        const URL = `/api/avg/${start}/${end}`
        try {
            const response = await fetch(URL, { method: 'GET', signal: signal })
            if (response.ok) {
                const data = await response.json()
                if (data["status"] == "found") {
                    return data["data"]
                }
            }
        } catch (err) {
            console.error('getAverage error: ', err.message)
        }
        return 0
    }

    return { 
        getReserveInRange,
        getAverage
    }
}, { persist: true })