<template>
  <v-container fluid>
    <!-- Area Chart -->
    <v-row>
      <v-col cols="1">
        <v-sheet class="pa-2 d-flex flex-column align-center justify-center" height="300" color="success" rounded>
          <v-slider v-model="waterLevel" direction="vertical" :min="0" :max="100" readonly color="white" thumb-label>
            <template v-slot:thumb-label="{ modelValue }">{{ modelValue }}</template>
          </v-slider>
          <span class="text-caption text-white mt-2">Height(in)</span>
        </v-sheet>
      </v-col>
      <v-col cols="11">
        <v-sheet elevation="2" rounded>
          <figure class="highcharts-figure">
            <div id="areaChart"></div>
          </figure>
        </v-sheet>
      </v-col>
    </v-row>

    <!-- Gauge and Liquid Progress -->
    <v-row class="mt-4">
      <v-col cols="12" md="8">
        <v-sheet elevation="2" rounded>
          <figure class="highcharts-figure">
            <div id="gaugeChart"></div>
          </figure>
        </v-sheet>
      </v-col>
      <v-col cols="12" md="4">
        <v-card elevation="2" height="100%">
          <v-card-title class="text-center text-primary">Water Level</v-card-title>
          <v-card-subtitle class="text-center">Capacity Remaining</v-card-subtitle>
          <v-card-text class="d-flex justify-center align-center" style="height:250px">
            <div class="liquid-container">
              <div class="liquid-fill" :style="{ height: percentage + '%', backgroundColor: liquidColor }"></div>
              <span class="liquid-text">{{ percentage }}%</span>
            </div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Overflow Dialog -->
    <v-dialog v-model="overflowDialog" max-width="400">
      <v-card>
        <v-card-title class="text-red">⚠ Overflow Detected</v-card-title>
        <v-card-text>The water level has exceeded 100% capacity!</v-card-text>
        <v-card-actions>
          <v-btn color="primary" @click="overflowDialog = false">Dismiss</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-container>
</template>

<script setup>
import { ref, watch, onMounted, onBeforeUnmount, computed } from 'vue'
import { useMqttStore } from '@/store/mqttStore'
import { storeToRefs } from 'pinia'
import Highcharts from 'highcharts'
import more from 'highcharts/highcharts-more'
import Exporting from 'highcharts/modules/exporting'
Exporting(Highcharts)
more(Highcharts)

const Mqtt = useMqttStore()
const { payload, payloadTopic } = storeToRefs(Mqtt)

const waterLevel = ref(0)
const percentage = ref(0)
const overflowDialog = ref(false)
const areaChartObj = ref(null)
const gaugeChartObj = ref(null)
const points = ref(600)
const shift = ref(false)

const liquidColor = computed(() => {
  if (percentage.value > 75) return '#1565C0'
  if (percentage.value > 40) return '#1E88E5'
  return '#64B5F6'
})

const createCharts = () => {
  areaChartObj.value = Highcharts.chart('areaChart', {
    chart: { zoomType: 'x' },
    title: { text: 'Water Reserves (10 min)', align: 'left' },
    yAxis: {
      title: { text: 'Water level' },
      labels: { format: '{value} Gal' }
    },
    xAxis: {
      type: 'datetime',
      title: { text: 'Time' }
    },
    tooltip: { shared: true },
    series: [{ name: 'Water', type: 'area', data: [], turboThreshold: 0, color: '#64B5F6' }]
  })

  gaugeChartObj.value = Highcharts.chart('gaugeChart', {
    chart: { type: 'gauge' },
    title: { text: 'Water Reserves', align: 'left' },
    yAxis: {
      min: 0, max: 1000,
      tickPixelInterval: 72,
      tickPosition: 'inside',
      tickColor: '#FFFFFF',
      tickLength: 20, tickWidth: 2,
      minorTickInterval: null,
      labels: { distance: 20, style: { fontSize: '14px' } },
      lineWidth: 0,
      plotBands: [
        { from: 0, to: 200, color: '#DF5353', thickness: 20 },
        { from: 200, to: 600, color: '#DDDF0D', thickness: 20 },
        { from: 600, to: 1000, color: '#55BF3B', thickness: 20 }
      ]
    },
    tooltip: { shared: true },
    pane: { startAngle: -90, endAngle: 89.9, background: null, center: ['50%', '75%'], size: '110%' },
    series: [{
      type: 'gauge', name: 'Water Capacity', data: [0],
      tooltip: { valueSuffix: ' Gal' },
      dataLabels: { format: '{y} Gal', borderWidth: 0, style: { fontSize: '16px' } },
      dial: { radius: '80%', backgroundColor: 'gray', baseWidth: 12, baseLength: '0%', rearLength: '0%' },
      pivot: { backgroundColor: 'gray', radius: 6 }
    }]
  })
}

watch(payload, (data) => {
  if (!data || data.type !== 'ultrasonic') return

  waterLevel.value = parseFloat(data.waterheight?.toFixed(1)) || 0
  percentage.value = parseFloat(data.percentage?.toFixed(1)) || 0

  if (percentage.value > 100) overflowDialog.value = true

  if (points.value > 0) { points.value-- }
  else { shift.value = true }

  if (areaChartObj.value) {
    areaChartObj.value.series[0].addPoint(
      { x: data.timestamp * 1000, y: parseFloat(data.reserve?.toFixed(2)) || 0 },
      true, shift.value
    )
  }
  if (gaugeChartObj.value) {
    gaugeChartObj.value.series[0].points[0].update(parseFloat(data.reserve?.toFixed(2)) || 0)
  }
})

onMounted(() => {
  createCharts()
  Mqtt.connect()
  setTimeout(() => {
    Mqtt.subscribe(String(620162191))
  }, 3000)
})

onBeforeUnmount(() => {
  Mqtt.unsubcribeAll()
})
</script>

<style>
figure { border: 1px solid #ccc; margin: 0; }
.liquid-container {
  position: relative;
  width: 150px;
  height: 150px;
  border-radius: 50%;
  border: 3px solid #1565C0;
  overflow: hidden;
  background: #e3f2fd;
}
.liquid-fill {
  position: absolute;
  bottom: 0;
  width: 100%;
  transition: height 0.5s ease;
}
.liquid-text {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  font-size: 1.5rem;
  font-weight: bold;
  color: #1565C0;
}
</style>