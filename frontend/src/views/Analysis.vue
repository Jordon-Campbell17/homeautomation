<template>
  <v-container fluid>
    <v-row>
      <!-- Date Range and Average -->
      <v-col cols="12" md="4">
        <v-row>
          <v-col cols="12">
            <v-text-field v-model="startDate" label="Start Date" type="date" density="compact" variant="outlined"></v-text-field>
          </v-col>
          <v-col cols="12">
            <v-text-field v-model="endDate" label="End Date" type="date" density="compact" variant="outlined"></v-text-field>
          </v-col>
          <v-col cols="12">
            <v-btn color="primary" @click="analyze">Analyze</v-btn>
          </v-col>
        </v-row>
      </v-col>

      <!-- Average Card -->
      <v-col cols="12" md="8">
        <v-card elevation="2" class="pa-4">
          <v-card-title class="text-center">Average</v-card-title>
          <v-card-subtitle class="text-center">For the selected period</v-card-subtitle>
          <v-card-text class="text-center">
            <span style="font-size: 4rem; font-weight: bold;">{{ average }}</span>
            <span class="text-h6 ml-2">Gal</span>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Line Chart -->
    <v-row class="mt-4">
      <v-col cols="12">
        <v-sheet elevation="2" rounded>
          <figure class="highcharts-figure">
            <div id="lineChart"></div>
          </figure>
        </v-sheet>
      </v-col>
    </v-row>

    <!-- Scatter Chart -->
    <v-row class="mt-4">
      <v-col cols="12">
        <v-sheet elevation="2" rounded>
          <figure class="highcharts-figure">
            <div id="scatterChart"></div>
          </figure>
        </v-sheet>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAppStore } from '@/store/appStore'
import Highcharts from 'highcharts'
import more from 'highcharts/highcharts-more'
import Exporting from 'highcharts/modules/exporting'
Exporting(Highcharts)
more(Highcharts)

const AppStore = useAppStore()
const startDate = ref('')
const endDate = ref('')
const average = ref(0)
const lineChartObj = ref(null)
const scatterChartObj = ref(null)

const createCharts = () => {
  lineChartObj.value = Highcharts.chart('lineChart', {
    chart: { zoomType: 'x' },
    title: { text: 'Water Management Analysis', align: 'left' },
    yAxis: {
      title: { text: 'Water Reserve' },
      labels: { format: '{value} Gal' }
    },
    xAxis: {
      type: 'datetime',
      title: { text: 'Time' }
    },
    tooltip: { shared: true },
    series: [{ name: 'Reserve', type: 'line', data: [], turboThreshold: 0, color: '#64B5F6' }]
  })

  scatterChartObj.value = Highcharts.chart('scatterChart', {
    chart: { type: 'scatter', zoomType: 'xy' },
    title: { text: 'Height and Water Level Correlation Analysis', align: 'left' },
    xAxis: {
      title: { text: 'Water Height' },
      labels: { format: '{value} in' }
    },
    yAxis: {
      title: { text: 'Height' },
      labels: { format: '{value} in' }
    },
    tooltip: { shared: true },
    series: [{ name: 'Analysis', type: 'scatter', data: [], turboThreshold: 0, color: '#1E88E5' }]
  })
}

const analyze = async () => {
  if (!startDate.value || !endDate.value) return

  const start = new Date(startDate.value).getTime() / 1000
  const end = new Date(endDate.value).getTime() / 1000

  // Fetch average
  try {
    const avgResponse = await fetch(`/api/avg/${start}/${end}`)
    const avgData = await avgResponse.json()
    if (avgData.status === 'found') {
      average.value = parseFloat(avgData.data.toFixed(1))
    }
  } catch (e) {
    console.log('avg error', e)
  }

  // Fetch data for charts
  try {
    const response = await fetch(`/api/reserve/${start}/${end}`)
    const data = await response.json()
    if (data.status === 'found') {
      const reserveData = []
      const scatterData = []

      data.data.forEach(row => {
        reserveData.push({ x: row.timestamp * 1000, y: parseFloat(row.reserve?.toFixed(2)) || 0 })
        scatterData.push([parseFloat(row.waterheight?.toFixed(2)) || 0, parseFloat(row.radar?.toFixed(2)) || 0])
      })

      lineChartObj.value.series[0].setData(reserveData)
      scatterChartObj.value.series[0].setData(scatterData)
    }
  } catch (e) {
    console.log('reserve error', e)
  }
}

onMounted(() => {
  createCharts()
})
</script>

<style>
figure { border: 1px solid #ccc; margin: 0; }
</style>