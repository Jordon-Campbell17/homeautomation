<template>
  <v-container fluid>
    <v-row justify="center">
      <v-col cols="12" md="6">
        <v-card class="pa-6" elevation="2">
          <v-card-title class="text-center">Combination</v-card-title>
          <v-card-subtitle class="text-center">Enter your four digit passcode</v-card-subtitle>
          <v-card-text class="d-flex justify-center mt-4">
            <v-otp-input v-model="passcode" length="4" type="number"></v-otp-input>
          </v-card-text>
          <v-card-actions class="justify-center">
            <v-btn color="primary" variant="flat" @click="submitPasscode">Submit</v-btn>
          </v-card-actions>
          <v-card-text class="text-center" v-if="message">
            <span :class="message.includes('success') ? 'text-green' : 'text-red'">{{ message }}</span>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { ref } from 'vue'

const passcode = ref('')
const message = ref('')

const submitPasscode = async () => {
  if (passcode.value.length !== 4) {
    message.value = 'Please enter a 4 digit passcode'
    return
  }
  try {
    const formData = new FormData()
    formData.append('passcode', passcode.value)
    const response = await fetch('/api/set/combination', {
      method: 'POST',
      body: formData
    })
    const data = await response.json()
    if (data.status === 'complete') {
      message.value = 'Passcode set successfully!'
    } else {
      message.value = 'Failed to set passcode'
    }
  } catch (error) {
    message.value = 'Error connecting to server'
  }
}
</script>