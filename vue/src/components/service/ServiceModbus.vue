<template>
  <div class="flex flex-col gap-4">
    <VCard variant="glass" padding="lg" class="relative has-[.z-30]:z-20">
      <div class="flex flex-wrap items-start justify-between gap-4">
        <h2 class="text-lg font-semibold">Modbus RTU</h2>

        <div class="flex flex-wrap gap-2">
          <VButton type="icon" variant="outline" :title="$t('modbus.applyConnection')" :aria-label="$t('modbus.applyConnection')" @click="onApply">
            <VIcon name="check" :size="16" />
          </VButton>
          <VButton type="icon" color="blue" :title="$t('modbus.saveConnection')" :aria-label="$t('modbus.saveConnection')" @click="onSave">
            <VIcon name="save" :size="16" />
          </VButton>
        </div>
      </div>

      <p v-if="dirty" class="mt-2 text-xs text-amber-600 dark:text-amber-400">{{ $t('modbus.unsaved') }}</p>

      <div class="mt-4 grid grid-cols-1 md:gap-x-4 sm:grid-cols-2" :class="form.role === 2 ? 'lg:grid-cols-4' : 'lg:grid-cols-5'">
        <VSelect class="sm:col-span-2 lg:col-span-1" :model-value="roleName" :items="roles" :label="$t('modbus.role')" @change="onRole" />
        <VSelect :model-value="baudName" :items="bauds" :disabled="form.role === 0" :label="$t('modbus.baud')" @change="onBaud" />
        <VSelect :model-value="parityName" :items="parities" :disabled="form.role === 0" :label="$t('modbus.parity')" @change="onParity" />
        <VSelect :model-value="stopBitsName" :items="stopBits" :disabled="form.role === 0" :label="$t('modbus.stopBits')" @change="onStopBits" />
        <VTextField v-if="form.role !== 2" :model-value="String(form.address)" :disabled="form.role !== 1" :label="$t('modbus.address')" @update:model-value="onAddress" />
      </div>

      <p class="mt-3 text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.format') }}: {{ formatText }} · {{ $t('modbus.pins') }}: TX 13 / RX 16 · {{ $t('modbus.autoDirection') }}</p>

      <div v-if="form.role !== 0" class="mt-4 border-t border-gray-light-10/70 pt-4 dark:border-white/10">
        <div class="grid grid-cols-2 gap-x-4 gap-y-3 sm:grid-cols-3 lg:grid-cols-6">
          <div v-for="item in statusItems" :key="item.label">
            <div class="text-xs text-gray-dark-25 dark:text-gray-light-10">{{ item.label }}</div>
            <div class="mt-1 text-sm font-semibold" :class="item.className">{{ item.value }}</div>
          </div>
        </div>
      </div>
    </VCard>

    <template v-if="form.role !== 0">
      <VCard variant="glass" padding="lg">
        <div class="flex flex-wrap items-center justify-between gap-4 min-h-10">
          <h3 class="text-base font-semibold">{{ $t('modbus.profile') }}</h3>

          <div class="flex flex-wrap items-center gap-2">
            <VButton
              type="icon"
              variant="outline"
              :disabled="profileLoading || profileSending !== null"
              :title="$t('modbus.importProfile')"
              :aria-label="$t('modbus.importProfile')"
              @click="fileInput?.click()"
            >
              <VIcon name="upload" :size="16" />
            </VButton>
            <VButton type="icon" variant="outline" :disabled="profileLoading || profileSending !== null" :title="$t('modbus.exportProfile')" :aria-label="$t('modbus.exportProfile')" @click="onExport">
              <VIcon name="download" :size="16" />
            </VButton>
            <VButton
              type="icon"
              variant="outline"
              :disabled="!profileValid || profileLoading || profileSending !== null"
              :loading="profileSending === 'apply'"
              :title="$t('modbus.applyProfile')"
              :aria-label="$t('modbus.applyProfile')"
              @click="onApplyProfile"
            >
              <VIcon name="check" :size="16" />
            </VButton>
            <VButton
              type="icon"
              color="blue"
              :disabled="!profileValid || profileLoading || profileSending !== null"
              :loading="profileSending === 'save'"
              :title="$t('modbus.saveProfile')"
              :aria-label="$t('modbus.saveProfile')"
              @click="onSaveProfile"
            >
              <VIcon name="save" :size="16" />
            </VButton>
          </div>
        </div>

        <div class="mt-2 flex flex-wrap items-center justify-end gap-2 text-xs">
          <p v-if="profileLoading" class="whitespace-nowrap text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.profileLoading') }}</p>
          <p v-else-if="profileSending" class="whitespace-nowrap text-amber-600 dark:text-amber-400">{{ profileSendingText }}</p>
          <p v-else-if="profileDirty" class="whitespace-nowrap text-amber-600 dark:text-amber-400">{{ $t('modbus.profileDirty') }}</p>
          <p v-else-if="profileMessage" class="whitespace-nowrap text-green-600 dark:text-green-400">{{ profileMessage }}</p>
          <p v-if="profileErrorText" class="text-red-600 dark:text-red-400">{{ profileErrorText }}</p>
        </div>

        <input ref="fileInput" class="hidden" type="file" accept=".bin,application/octet-stream" @change="onImport" />

        <template v-if="form.role === 2">
          <div class="mt-6 flex items-center justify-between gap-4">
            <h4 class="text-sm font-semibold">
              {{ $t('modbus.devices') }}
              <span class="ml-1 text-xs font-normal text-gray-dark-25 dark:text-gray-light-10">{{ draft.deviceCount }}/8</span>
            </h4>
            <VButton
              type="icon"
              variant="outline"
              :disabled="profileLoading || profileSending !== null || draft.deviceCount >= 8"
              :title="$t('modbus.addDevice')"
              :aria-label="$t('modbus.addDevice')"
              @click="openDevice()"
            >
              <VIcon name="add" :size="16" />
            </VButton>
          </div>

          <div class="mt-2 flex flex-col gap-2">
            <div
              v-for="(device, index) in draft.devices.slice(0, draft.deviceCount)"
              :key="`device-${index}`"
              class="flex flex-wrap items-center gap-3 rounded-lg bg-black/5 px-3 py-2 dark:bg-white/5"
            >
              <div class="min-w-0 flex-1">
                <div class="truncate text-sm font-medium">{{ device.name || device.id || `#${index + 1}` }}</div>
                <div class="truncate text-xs text-gray-dark-25 dark:text-gray-light-10">
                  {{ device.id }} · {{ $t('modbus.address') }} {{ device.address }} · {{ device.interval }} ms · {{ $t('modbus.timeout') }} {{ device.timeout }} ms · {{ $t('modbus.retries') }}
                  {{ device.retries }} ·
                  {{ device.enabled ? $t('modbus.enabled') : $t('modbus.disabledState') }}
                </div>
              </div>
              <div>
                <VButton type="icon" variant="ghost" :title="$t('modbus.edit')" :aria-label="$t('modbus.edit')" @click="openDevice(index)">
                  <VIcon name="edit" :size="16" />
                </VButton>
                <VButton type="icon" variant="ghost" color="red" :title="$t('modbus.remove')" :aria-label="$t('modbus.remove')" @click="askRemoveDevice(index)">
                  <VIcon name="trash" :size="16" />
                </VButton>
              </div>
            </div>
            <p v-if="!profileLoading && draft.deviceCount === 0" class="text-sm text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.noDevices') }}</p>
          </div>

          <div class="mt-6 flex items-center justify-between gap-4">
            <h4 class="text-sm font-semibold">
              {{ $t('modbus.masterPoints') }}
              <span class="ml-1 text-xs font-normal text-gray-dark-25 dark:text-gray-light-10">{{ masterPointEntries.length }} · {{ draft.pointCount }}/64</span>
            </h4>
            <VButton
              type="icon"
              variant="outline"
              :disabled="profileLoading || profileSending !== null || draft.pointCount >= 64"
              :title="$t('modbus.addPoint')"
              :aria-label="$t('modbus.addPoint')"
              @click="openPoint()"
            >
              <VIcon name="add" :size="16" />
            </VButton>
          </div>

          <div class="mt-2 flex flex-col gap-2">
            <div v-for="entry in masterPointEntries" :key="`point-${entry.index}`" class="flex flex-wrap items-center gap-3 rounded-lg bg-black/5 px-3 py-2 dark:bg-white/5">
              <div class="min-w-0 flex-1">
                <div class="truncate text-sm font-medium">{{ entry.point.name || entry.point.id || `#${entry.index + 1}` }}</div>
                <div class="truncate text-xs text-gray-dark-25 dark:text-gray-light-10">
                  {{ deviceName(entry.point.device) }} · FC {{ entry.point.fc }} · {{ $t('modbus.address') }} {{ entry.point.address }} · {{ typeName(entry.point.type) }} ·
                  {{ entry.point.var === 0xff ? '—' : varName(entry.point.var) }}
                </div>
              </div>

              <div class="flex items-center gap-2">
                <span class="text-sm font-medium">{{ pointValue(entry.point) }}</span>
                <span class="rounded px-2 py-1 text-xs font-semibold" :class="qualityClass(liveOf(entry.point)?.quality)">
                  {{ qualityName(liveOf(entry.point)?.quality) }}
                </span>
              </div>

              <template v-if="entry.point.flags & 4">
                <VTextField hide-message class="w-28" :model-value="writeDrafts[entry.index] ?? ''" :label="$t('modbus.writeValue')" @update:model-value="(v) => (writeDrafts[entry.index] = v)" />
                <VButton type="icon" variant="soft" :title="$t('modbus.write')" :aria-label="$t('modbus.write')" @click="writePoint(entry.index)">
                  <VIcon name="play-circle" :size="16" />
                </VButton>
              </template>

              <VButton type="icon" variant="ghost" :title="$t('modbus.edit')" :aria-label="$t('modbus.edit')" @click="openPoint(entry.index)">
                <VIcon name="edit" :size="16" />
              </VButton>
              <VButton type="icon" variant="ghost" color="red" :title="$t('modbus.remove')" :aria-label="$t('modbus.remove')" @click="askRemovePoint(entry.index)">
                <VIcon name="trash" :size="16" />
              </VButton>
            </div>
            <p v-if="!profileLoading && masterPointEntries.length === 0" class="text-sm text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.noPoints') }}</p>
          </div>
        </template>

        <template v-if="form.role === 1">
          <div class="mt-6 flex items-center justify-between gap-4">
            <h4 class="text-sm font-semibold">
              {{ $t('modbus.slaveValues') }}
              <span class="ml-1 text-xs font-normal text-gray-dark-25 dark:text-gray-light-10">
                {{ $t('modbus.slaveValue') }} {{ slavePointEntries.length }} · {{ $t('modbus.bankUsage', { used: slaveBankUsed, total: SLAVE_REG_COUNT }) }}
              </span>
            </h4>
            <div class="flex gap-2">
              <VButton
                type="icon"
                variant="outline"
                :disabled="profileLoading || profileSending !== null || draft.pointCount >= 64"
                :title="$t('modbus.addSlaveValue')"
                :aria-label="$t('modbus.addSlaveValue')"
                @click="openSlaveValue"
              >
                <VIcon name="add" :size="16" />
              </VButton>
            </div>
          </div>

          <div class="mt-2 flex flex-col gap-2">
            <div v-for="entry in slavePointEntries" :key="`slave-${entry.index}`" class="flex flex-wrap items-center gap-3 rounded-lg bg-black/5 px-3 py-2 dark:bg-white/5">
              <div class="min-w-0 flex-1">
                <div class="truncate text-sm font-medium">{{ entry.point.name || entry.point.id || `#${entry.index + 1}` }}</div>
                <div class="truncate text-xs text-gray-dark-25 dark:text-gray-light-10">
                  {{ $t('modbus.slaveValue') }} · {{ $t('modbus.address') }} {{ entry.point.address }}–{{
                    entry.point.address + Math.max(1, entry.point.registers || registersForType(entry.point.type, 1)) - 1
                  }}
                  · {{ typeName(entry.point.type) }} · {{ varSourceLabel(draft.vars[entry.point.var]) }}
                </div>
              </div>
              <div class="flex items-center gap-2">
                <span class="text-sm font-medium">{{ pointValue(entry.point) }}</span>
                <span class="rounded px-2 py-1 text-xs font-semibold" :class="qualityClass(liveOf(entry.point)?.quality)">
                  {{ qualityName(liveOf(entry.point)?.quality) }}
                </span>
              </div>
              <VButton type="icon" variant="ghost" :title="$t('modbus.edit')" :aria-label="$t('modbus.edit')" @click="openSlaveValueEdit(entry.index)">
                <VIcon name="edit" :size="16" />
              </VButton>
              <VButton type="icon" variant="ghost" color="red" :title="$t('modbus.remove')" :aria-label="$t('modbus.remove')" @click="askRemovePoint(entry.index)">
                <VIcon name="trash" :size="16" />
              </VButton>
            </div>

            <p v-if="!profileLoading && slavePointEntries.length === 0" class="text-sm text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.noSlaveValues') }}</p>
          </div>
        </template>

        <template v-if="form.role === 2">
          <div class="mt-6 flex items-center justify-between gap-4">
            <h4 class="text-sm font-semibold">
              {{ $t('modbus.vars') }}
              <span class="ml-1 text-xs font-normal text-gray-dark-25 dark:text-gray-light-10">{{ draft.varCount }}/32</span>
            </h4>
            <VButton
              type="icon"
              variant="outline"
              :disabled="profileLoading || profileSending !== null || draft.varCount >= 32"
              :title="$t('modbus.addVar')"
              :aria-label="$t('modbus.addVar')"
              @click="openVar()"
            >
              <VIcon name="add" :size="16" />
            </VButton>
          </div>

          <div class="mt-2 flex flex-col gap-2">
            <div v-for="(variable, index) in draft.vars.slice(0, draft.varCount)" :key="`var-${index}`" class="flex flex-wrap items-center gap-3 rounded-lg bg-black/5 px-3 py-2 dark:bg-white/5">
              <div class="min-w-0 flex-1">
                <div class="truncate text-sm font-medium">{{ variable.name || variable.id || `#${index + 1}` }}</div>
                <div class="truncate text-xs text-gray-dark-25 dark:text-gray-light-10">
                  {{ variable.id }} · {{ typeName(variable.type) }} · {{ varSourceLabel(variable) }} · vars.{{ variable.id }}.value
                </div>
              </div>

              <div class="flex items-center gap-2">
                <span class="text-sm font-medium">{{ varValue(variable, index) }}</span>
                <span class="rounded px-2 py-1 text-xs font-semibold" :class="qualityClass(main.modbus.vars[index]?.quality)">
                  {{ qualityName(main.modbus.vars[index]?.quality) }}
                </span>
              </div>

              <VButton type="icon" variant="ghost" :title="$t('modbus.edit')" :aria-label="$t('modbus.edit')" @click="openVar(index)">
                <VIcon name="edit" :size="16" />
              </VButton>
              <VButton type="icon" variant="ghost" color="red" :title="$t('modbus.remove')" :aria-label="$t('modbus.remove')" @click="askRemoveVar(index)">
                <VIcon name="trash" :size="16" />
              </VButton>
            </div>
            <p v-if="!profileLoading && draft.varCount === 0" class="text-sm text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.noVars') }}</p>
          </div>
        </template>
      </VCard>
    </template>

    <VCard v-if="form.role === 1" variant="glass" padding="lg">
      <h3 class="text-base font-semibold">{{ $t('modbus.map') }}</h3>

      <VTable class="mt-4" :headers="mapHeaders" :items="mapItems" @click="onMapClick">
        <template #type="{ item }">
          <div class="text-gray-dark-25 dark:text-gray-light-10">{{ item.type }}</div>
        </template>
        <template #target="{ item }">
          <div class="flex flex-col">
            <span>{{ item.target }}</span>
          </div>
        </template>
      </VTable>

      <ModbusMapDetails v-if="mapDetails && isDesktop" class="mt-4 border-t border-gray-light-10/70 pt-4 dark:border-white/10" :details="mapDetails" closable @close="closeMapDetails" />
    </VCard>

    <VDialog v-if="mapDetails && !isDesktop" size="md" :title="$t('modbus.mapDetails')" @close="closeMapDetails">
      <ModbusMapDetails :details="mapDetails" />
    </VDialog>

    <VDialog v-if="deviceEdit !== null" size="md" :title="deviceEdit < 0 ? $t('modbus.addDevice') : $t('modbus.editDevice')" @close="deviceEdit = null">
      <form class="flex max-h-[70dvh] xl:max-h-[calc(100dvh-27rem)] min-h-0 flex-col gap-4" @submit.prevent="saveDevice" @keydown.esc.stop="deviceEdit = null">
        <div class="flex-1">
          <div class="grid grid-cols-1 gap-2 sm:grid-cols-2 lg:grid-cols-3">
            <VTextField :error="deviceErrors.id" :model-value="deviceForm.id" :label="$t('modbus.id')" @update:model-value="(v) => (deviceForm.id = v)" />
            <VTextField :error="deviceErrors.name" :model-value="deviceForm.name" :label="$t('name')" @update:model-value="(v) => (deviceForm.name = v)" />
            <VTextField :error="deviceErrors.address" :model-value="String(deviceForm.address)" :label="$t('modbus.address')" @update:model-value="(v) => (deviceForm.address = toInt(v))" />
            <VSelect :model-value="selectName(enabledItems, deviceForm.enabled)" :items="enabledItems" :label="$t('modbus.enabled')" @change="({ value }) => (deviceForm.enabled = value)" />
            <VTextField :error="deviceErrors.interval" :model-value="String(deviceForm.interval)" :label="$t('modbus.interval')" @update:model-value="(v) => (deviceForm.interval = toInt(v))" />
            <VTextField :error="deviceErrors.timeout" :model-value="String(deviceForm.timeout)" :label="$t('modbus.timeout')" @update:model-value="(v) => (deviceForm.timeout = toInt(v))" />
            <VTextField :error="deviceErrors.retries" :model-value="String(deviceForm.retries)" :label="$t('modbus.retries')" @update:model-value="(v) => (deviceForm.retries = toInt(v))" />
          </div>
        </div>

        <div class="flex justify-end gap-3">
          <VButton class="min-w-24" color="blue" @click="saveDevice">{{ $t('modbus.apply') }}</VButton>
          <VButton class="min-w-24" variant="outline" color="gray" @click="deviceEdit = null">{{ $t('modbus.close') }}</VButton>
          <button type="submit" class="hidden" aria-hidden="true" tabindex="-1"></button>
        </div>
      </form>
    </VDialog>

    <VDialog v-if="slaveEdit !== null && !sourceTreeOpen" size="md" :title="slaveEdit < 0 ? $t('modbus.addSlaveValue') : $t('modbus.editSlaveValue')" @close="closeSlaveValue">
      <form class="flex max-h-[70dvh] xl:max-h-[calc(100dvh-27rem)] min-h-0 flex-col gap-4" @submit.prevent="saveSlaveValue" @keydown.esc.stop="closeSlaveValue">
        <div class="flex-1">
          <div class="grid grid-cols-1 gap-2 sm:grid-cols-2">
            <VSelect class="mb-4" hide-message :model-value="slaveSourceKindName" :items="slaveSourceKindItems" :label="$t('modbus.source')" @change="({ value }) => onSlaveSourceKind(value)" />
            <div v-if="slaveForm.sourceKind === 'data'" class="flex items-end gap-2 mb-4">
              <VTextField class="flex-auto" readonly hide-message :model-value="slaveForm.sourcePath || '—'" :label="$t('modbus.value')" />
              <VButton type="icon" variant="outline" :title="$t('modbus.pickData')" :aria-label="$t('modbus.pickData')" @click="openSourceTree">
                <VIcon name="search" :size="16" />
              </VButton>
            </div>
            <p v-else class="sm:col-span-2 text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.scriptVarHint', { id: slaveForm.id }) }}</p>
            <VTextField :error="slaveErrors.name" :model-value="slaveForm.name" :label="$t('name')" @update:model-value="(v) => (slaveForm.name = v)" />
            <VTextField :error="slaveErrors.address" :model-value="String(slaveForm.address)" :label="$t('modbus.address')" @update:model-value="(v) => (slaveForm.address = toInt(v))" />
            <VTextField v-if="slaveForm.sourceKind === 'data'" readonly :model-value="slaveTypeText" :label="$t('modbus.type')" />
            <VSelect v-else :model-value="selectName(slaveTypeItems, slaveForm.type)" :items="slaveTypeItems" :label="$t('modbus.type')" @change="({ value }) => (slaveForm.type = value)" />
            <VTextField :error="slaveErrors.decimals" :model-value="String(slaveForm.decimals)" :label="$t('modbus.decimals')" @update:model-value="(v) => (slaveForm.decimals = toInt(v))" />
            <VTextField :error="slaveErrors.unit" :model-value="slaveForm.unit" :label="$t('modbus.unit')" @update:model-value="(v) => (slaveForm.unit = v)" />
          </div>

          <div class="mt-3 flex flex-wrap gap-4">
            <VCheckbox :model-value="slaveForm.writable" @update:model-value="(v) => (slaveForm.writable = v)">{{ $t('modbus.writable') }}</VCheckbox>
          </div>
          <p v-if="slaveWritableHint" class="mt-1 text-xs text-amber-600 dark:text-amber-400">{{ $t('modbus.writableHint') }}</p>

          <VButton class="mt-3" variant="ghost" @click="slaveAdvanced = !slaveAdvanced">
            <div class="flex gap-3 items-center">
              {{ $t('modbus.advanced') }}
              <VIcon name="chevron-down" :size="16" :class="slaveAdvanced ? 'rotate-180' : ''" />
            </div>
          </VButton>
          <div v-if="slaveAdvanced" class="mt-2 grid grid-cols-1 gap-2 sm:grid-cols-2">
            <VTextField :error="slaveErrors.id" :model-value="slaveForm.id" :label="$t('modbus.id')" @update:model-value="(v) => (slaveForm.id = v)" />
            <VTextField :model-value="String(slaveForm.scale)" :label="$t('modbus.scale')" @update:model-value="(v) => (slaveForm.scale = toNumber(v, 1))" />
            <VTextField :model-value="String(slaveForm.offset)" :label="$t('modbus.offset')" @update:model-value="(v) => (slaveForm.offset = toNumber(v, 0))" />
          </div>

          <p v-if="slaveErrors.source || slaveErrors.address || slaveErrors.name || slaveErrors.id" class="mt-2 text-xs text-red-600 dark:text-red-400">
            {{ slaveErrors.source || slaveErrors.address || slaveErrors.name || slaveErrors.id }}
          </p>
        </div>

        <div class="flex justify-end gap-3">
          <VButton class="min-w-24" color="blue" type="submit" @click="saveSlaveValue">{{ $t('modbus.apply') }}</VButton>
          <VButton class="min-w-24" variant="outline" color="gray" @click="closeSlaveValue">{{ $t('modbus.close') }}</VButton>
        </div>
      </form>
    </VDialog>

    <VDialog v-if="sourceTreeOpen" size="md" :title="$t('modbus.pickData')" @close="closeSourceTree">
      <div class="relative max-h-120 overflow-auto scrollbar">
        <VListObject :items="sourceTree" only-value :can-add="canAddSource" @click="onSourcePath" />
      </div>
    </VDialog>

    <VDialog v-if="pointEdit !== null" size="lg" :title="pointEdit < 0 ? $t('modbus.addPoint') : $t('modbus.editPoint')" @close="closePoint">
      <form class="flex max-h-[70dvh] xl:max-h-[calc(100dvh-27rem)] min-h-0 flex-col gap-4" @submit.prevent="savePoint" @keydown.esc.stop="closePoint">
        <VTabs :key="pointTab" :value="pointTab" :items="pointTabs" size="sm" @click="onPointTab" />
        <div class="flex-1">
          <template v-if="pointTab === 0">
            <div class="grid grid-cols-1 gap-2 sm:grid-cols-2 lg:grid-cols-4">
              <VTextField :error="pointErrors.id" :model-value="pointForm.id" :label="$t('modbus.id')" @update:model-value="(v) => (pointForm.id = v)" />
              <VTextField :error="pointErrors.name" :model-value="pointForm.name" :label="$t('name')" @update:model-value="(v) => (pointForm.name = v)" />
              <VSelect :model-value="selectName(pointDeviceItems, pointForm.device)" :items="pointDeviceItems" :label="$t('modbus.device')" @change="({ value }) => (pointForm.device = value)" />
              <VSelect :model-value="selectName(fcItems, pointForm.fc)" :items="fcItems" :label="$t('modbus.fc')" @change="({ value }) => (pointForm.fc = value)" />
              <VTextField :error="pointErrors.address" :model-value="String(pointForm.address)" :label="$t('modbus.address')" @update:model-value="(v) => (pointForm.address = toInt(v))" />
              <VSelect :model-value="selectName(typeItems, pointForm.type)" :items="typeItems" :label="$t('modbus.type')" @change="({ value }) => onPointType(value)" />
              <VSelect :model-value="selectName(masterVarItems, pointForm.var)" :items="masterVarItems" :label="$t('modbus.variable')" @change="({ value }) => (pointForm.var = value)" />
              <VTextField :error="pointErrors.registers" :model-value="String(pointForm.registers)" :label="$t('modbus.registers')" @update:model-value="(v) => (pointForm.registers = toInt(v))" />
              <VTextField :error="pointErrors.decimals" :model-value="String(pointForm.decimals)" :label="$t('modbus.decimals')" @update:model-value="(v) => (pointForm.decimals = toInt(v))" />
              <VTextField :model-value="String(pointForm.scale)" :label="$t('modbus.scale')" @update:model-value="(v) => (pointForm.scale = toNumber(v, 1))" />
              <VTextField :model-value="String(pointForm.offset)" :label="$t('modbus.offset')" @update:model-value="(v) => (pointForm.offset = toNumber(v, 0))" />
              <VTextField :error="pointErrors.unit" :model-value="pointForm.unit" :label="$t('modbus.unit')" @update:model-value="(v) => (pointForm.unit = v)" />
            </div>

            <div class="flex flex-wrap gap-4">
              <VCheckbox :model-value="Boolean(pointForm.flags & 4)" @update:model-value="(v) => setFlag(pointForm, 4, v)">
                {{ $t('modbus.writable') }}
              </VCheckbox>
              <VCheckbox :model-value="Boolean(pointForm.flags & 2)" @update:model-value="(v) => setFlag(pointForm, 2, v)">
                {{ $t('modbus.byteLittle') }}
              </VCheckbox>
              <VCheckbox :model-value="Boolean(pointForm.flags & 1)" @update:model-value="(v) => setFlag(pointForm, 1, v)">
                {{ $t('modbus.wordLittle') }}
              </VCheckbox>
            </div>

            <p v-if="pointErrors.device || pointErrors.var" class="text-xs text-red-600 dark:text-red-400">
              {{ pointErrors.device || pointErrors.var }}
            </p>
          </template>

          <template v-else>
            <div class="flex flex-wrap items-start justify-between gap-2">
              <div>
                <h4 class="text-sm font-semibold">{{ $t('modbus.pointTest') }}</h4>
                <p class="mt-1 text-xs text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.pointTestHint') }}</p>
              </div>
              <span class="font-mono text-xs text-gray-dark-25 dark:text-gray-light-10">{{ testSummary }}</span>
            </div>

            <div class="mt-3">
              <VCheckbox :model-value="testAutoCrc" @update:model-value="setTestAutoCrc">{{ $t('modbus.rawAutoCrc') }}</VCheckbox>
            </div>

            <VTextField class="mt-2" hide-message :model-value="testFrame" :label="$t('modbus.rawRequest')" @update:model-value="onTestFrameInput" />

            <p class="mt-1 text-xs text-gray-dark-25 dark:text-gray-light-10">
              {{ testAutoCrc ? $t('modbus.pointTestAutoHint') : $t('modbus.pointTestManualHint') }}
            </p>

            <div class="mt-3 flex flex-wrap items-center gap-2">
              <VButton variant="outline" @click="rebuildTestFrame">{{ $t('modbus.rawBuild') }}</VButton>
              <VButton color="blue" :loading="testPending" :disabled="!testValid" @click="onPointTest">{{ $t('modbus.testRead') }}</VButton>
            </div>

            <p v-if="testError" class="mt-2 text-xs text-red-600 dark:text-red-400">{{ testError }}</p>

            <div v-if="testResult" ref="testResultRef" class="mt-3 grid grid-cols-1 gap-2 sm:grid-cols-2">
              <div>
                <div class="text-xs font-semibold text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.status') }}</div>
                <div class="mt-1 text-sm font-semibold" :class="testStatusClass(testResult.status)">{{ testStatusName(testResult.status) }}</div>
              </div>
              <div>
                <div class="text-xs font-semibold text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.rawElapsed') }}</div>
                <div class="mt-1 font-mono text-sm">{{ testResult.elapsed }}</div>
              </div>
              <div class="sm:col-span-2">
                <div class="text-xs font-semibold text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.rawResponse') }}</div>
                <div class="mt-1 break-all font-mono text-sm">{{ testResponse || $t('modbus.rawEmpty') }}</div>
              </div>
              <div class="sm:col-span-2">
                <div class="text-xs font-semibold text-gray-dark-25 dark:text-gray-light-10">{{ $t('modbus.testValue') }}</div>
                <div class="mt-1 text-sm font-medium">{{ testDecoded }}</div>
              </div>
              <div v-if="testException !== null" class="text-xs text-red-600 dark:text-red-400 sm:col-span-2">{{ $t('modbus.exceptionCode') }}: {{ testException }}</div>
            </div>
          </template>
        </div>

        <div class="flex justify-end gap-3">
          <VButton class="min-w-24" color="blue" @click="savePoint">{{ $t('modbus.apply') }}</VButton>
          <VButton class="min-w-24" variant="outline" color="gray" @click="pointEdit = null">{{ $t('modbus.close') }}</VButton>
          <button type="submit" class="hidden" aria-hidden="true" tabindex="-1"></button>
        </div>
      </form>
    </VDialog>

    <VDialog v-if="varEdit !== null" size="md" :title="varEdit < 0 ? $t('modbus.addVar') : $t('modbus.editVar')" @close="varEdit = null">
      <form class="flex max-h-[70dvh] xl:max-h-[calc(100dvh-27rem)] min-h-0 flex-col gap-4" @submit.prevent="saveVar" @keydown.esc.stop="varEdit = null">
        <div class="flex-1">
          <div class="grid grid-cols-1 gap-2 sm:grid-cols-2">
            <VTextField :error="varErrors.id" :model-value="varForm.id" :label="$t('modbus.id')" @update:model-value="(v) => (varForm.id = v)" />
            <VTextField :error="varErrors.name" :model-value="varForm.name" :label="$t('name')" @update:model-value="(v) => (varForm.name = v)" />
            <VSelect :model-value="selectName(typeItems, varForm.type)" :items="typeItems" :label="$t('modbus.type')" @change="({ value }) => onVarType(value)" />
            <VSelect
              :disabled="!!varForm.source"
              :model-value="selectName(varPointItems, varForm.point)"
              :items="varPointItems"
              :label="$t('modbus.point')"
              @change="({ value }) => (varForm.point = value)"
            />
            <VTextField readonly :model-value="varForm.source" :label="$t('modbus.source')" />
            <VTextField :error="varErrors.unit" :model-value="varForm.unit" :label="$t('modbus.unit')" @update:model-value="(v) => (varForm.unit = v)" />
            <VTextField :error="varErrors.decimals" :model-value="String(varForm.decimals)" :label="$t('modbus.decimals')" @update:model-value="(v) => (varForm.decimals = toInt(v))" />
          </div>

          <p v-if="varErrors.point" class="text-xs text-red-600 dark:text-red-400">
            {{ varErrors.point }}
          </p>
        </div>

        <div class="flex justify-end gap-3">
          <VButton class="min-w-24" color="blue" @click="saveVar">{{ $t('modbus.apply') }}</VButton>
          <VButton class="min-w-24" variant="outline" color="gray" @click="varEdit = null">{{ $t('modbus.close') }}</VButton>
          <button type="submit" class="hidden" aria-hidden="true" tabindex="-1"></button>
        </div>
      </form>
    </VDialog>

    <VDialog v-if="deleteTarget" size="sm" :title="$t('modbus.deleteTitle')" @close="deleteTarget = null">
      <p class="text-sm text-gray-dark-30 dark:text-gray-main">{{ deleteHint }}</p>
      <div class="mt-4 flex justify-end gap-3">
        <VButton class="min-w-24" color="red" @click="confirmDelete">{{ $t('modbus.remove') }}</VButton>
        <VButton class="min-w-24" variant="outline" color="gray" @click="deleteTarget = null">{{ $t('modbus.close') }}</VButton>
      </div>
    </VDialog>
  </div>
</template>

<script setup lang="ts">
import type { IListItem, IModbus, IModbusDevice, IModbusPoint, IModbusProfile, IModbusRaw, IModbusVar, TMessage } from '@/types';
import type { TCommand } from '@/generated/protocol';
import type { IMapDetails, IMapItem } from './modbusMapTypes';

import { useMediaQuery } from '@vueuse/core';

import { computed, nextTick, onUnmounted, reactive, ref, watch } from 'vue';

import { buildReadFrame, buildReadResponse, encodePointValue, formatHex } from '@/utils/modbusFrame';
import { KEY } from '@/utils/const';
import { CMD } from '@/generated/protocol';
import { struct } from '@/assets/js/';

import { useConnection } from '@/composables/useConnection';
import { useLocale } from '@/composables/useLocale';

import { VButton, VCard, VCheckbox, VDialog, VIcon, VListObject, VSelect, VTabs, VTable, VTextField } from 'vuesp-components/components';

import ModbusMapDetails from './ModbusMapDetails.vue';

interface IDeviceForm {
  id: string;
  name: string;
  address: number;
  enabled: number;
  interval: number;
  timeout: number;
  retries: number;
}

type TSlaveSourceKind = 'data' | 'script';

interface IPointForm {
  device: number;
  id: string;
  name: string;
  fc: number;
  address: number;
  type: number;
  flags: number;
  registers: number;
  decimals: number;
  scale: number;
  offset: number;
  unit: string;
  var: number;
}

interface IVarForm {
  id: string;
  name: string;
  type: number;
  flags: number;
  unit: string;
  decimals: number;
  point: number;
  source: string;
}

interface ISlaveForm {
  sourceKind: TSlaveSourceKind;
  sourcePath: string;
  id: string;
  name: string;
  address: number;
  type: number;
  scale: number;
  offset: number;
  decimals: number;
  unit: string;
  writable: boolean;
}

interface ISourceMeta {
  type: number;
  decimals?: number;
  unit?: string;
}

type TProfileAction = 'apply' | 'save';
type TDeleteKind = 'device' | 'point' | 'var';

interface IDeleteTarget {
  kind: TDeleteKind;
  index: number;
  name: string;
}

const { $t } = useLocale();

const TYPE_U16 = 0;
const TYPE_S16 = 1;
const TYPE_U32 = 2;
const TYPE_S32 = 3;
const TYPE_U64 = 4;
const TYPE_S64 = 5;
const TYPE_FLOAT32 = 6;
const TYPE_FLOAT64 = 7;
const TYPE_BOOL = 8;
const TYPE_STRING = 9;
const TYPE_BIT = 10;
const TYPE_ENUM = 11;
const TYPE_BCD = 12;

const NONE = 255;
const DEVICE_SLAVE = 0xff;
const SLAVE_ADDR_START = 100;
const SLAVE_REG_COUNT = 32;

const roles: IListItem<number>[] = [
  { name: $t('modbus.off'), value: 0 },
  { name: $t('modbus.slave'), value: 1 },
  { name: $t('modbus.master'), value: 2 },
];

const bauds: IListItem<number>[] = [
  { name: '4800', value: 4800 },
  { name: '9600', value: 9600 },
  { name: '19200', value: 19200 },
];

const parities: IListItem<number>[] = [
  { name: $t('modbus.parityNone'), value: 0 },
  { name: $t('modbus.parityEven'), value: 1 },
  { name: $t('modbus.parityOdd'), value: 2 },
];

const stopBits: IListItem<number>[] = [
  { name: '1', value: 1 },
  { name: '2', value: 2 },
];

const enabledItems: IListItem<number>[] = [
  { name: $t('modbus.enabled'), value: 1 },
  { name: $t('modbus.disabledState'), value: 0 },
];

const fcItems: IListItem<number>[] = [
  { name: '01 · Coils', value: 1 },
  { name: '02 · Discrete', value: 2 },
  { name: '03 · Holding', value: 3 },
  { name: '04 · Input', value: 4 },
];

const SOURCE_FIELDS: Record<string, Record<string, ISourceMeta>> = {
  ports: {
    key: { type: TYPE_U16 },
    gpio: { type: TYPE_U16 },
    value: { type: TYPE_U16 },
    mode: { type: TYPE_U16 },
    interrupt: { type: TYPE_U16 },
    list: { type: TYPE_U16 },
    disabled: { type: TYPE_U16 },
    isButton: { type: TYPE_U16 },
    command: { type: TYPE_U16 },
    isPressed: { type: TYPE_U16 },
    count: { type: TYPE_U16 },
    empty: { type: TYPE_U16 },
    valueOld: { type: TYPE_U16 },
    pressStart: { type: TYPE_U32 },
  },
  device: {
    command: { type: TYPE_U16 },
    input: { type: TYPE_U16 },
    output: { type: TYPE_U16 },
    analog: { type: TYPE_U16 },
    now: { type: TYPE_U32 },
    list: { type: TYPE_U32 },
  },
  info: {
    totalBytes: { type: TYPE_U32 },
    usedBytes: { type: TYPE_U32 },
    id: { type: TYPE_U32 },
    uptime: { type: TYPE_U32 },
    board: { type: TYPE_U32 },
    freeHeap: { type: TYPE_U32 },
    minFreeHeap: { type: TYPE_U32 },
    maxBlock: { type: TYPE_U32 },
    stackFree: { type: TYPE_U32 },
    loopLastUs: { type: TYPE_U32 },
    loopMaxUs: { type: TYPE_U32 },
    loopP95Us: { type: TYPE_U32 },
    loopCount: { type: TYPE_U32 },
    fragmentation: { type: TYPE_U16 },
    clients: { type: TYPE_U16 },
    scripts: { type: TYPE_U16 },
    peers: { type: TYPE_U16 },
    wifi: { type: TYPE_U16 },
    diagnostics: { type: TYPE_U16 },
  },
  modbus: {
    role: { type: TYPE_U16 },
    address: { type: TYPE_U16 },
    baud: { type: TYPE_U32 },
    dataBits: { type: TYPE_U16 },
    parity: { type: TYPE_U16 },
    stopBits: { type: TYPE_U16 },
    status: { type: TYPE_U16 },
    lastFunction: { type: TYPE_U16 },
    lastAddress: { type: TYPE_U16 },
    lastError: { type: TYPE_U16 },
    requests: { type: TYPE_U32 },
    errors: { type: TYPE_U32 },
    varCount: { type: TYPE_U16 },
    updated: { type: TYPE_U32 },
    point: { type: TYPE_U16 },
    ok: { type: TYPE_U16 },
    value: { type: TYPE_FLOAT32 },
  },
  dallas: {
    temp: { type: TYPE_FLOAT32, decimals: 2, unit: '°C' },
  },
};

const typeItems: IListItem<number>[] = [
  { name: 'U16', value: TYPE_U16 },
  { name: 'S16', value: TYPE_S16 },
  { name: 'U32', value: TYPE_U32 },
  { name: 'S32', value: TYPE_S32 },
  { name: 'U64', value: TYPE_U64 },
  { name: 'S64', value: TYPE_S64 },
  { name: 'Float32', value: TYPE_FLOAT32 },
  { name: 'Float64', value: TYPE_FLOAT64 },
  { name: 'Bool', value: TYPE_BOOL },
  { name: 'String', value: TYPE_STRING },
  { name: 'Bit', value: TYPE_BIT },
  { name: 'Enum', value: TYPE_ENUM },
  { name: 'BCD', value: TYPE_BCD },
];

const slaveTypeItems: IListItem<number>[] = [
  { name: 'U16', value: TYPE_U16 },
  { name: 'S16', value: TYPE_S16 },
  { name: 'U32', value: TYPE_U32 },
  { name: 'S32', value: TYPE_S32 },
  { name: 'Float32', value: TYPE_FLOAT32 },
];

const slaveSourceKindItems: IListItem<TSlaveSourceKind>[] = [
  { name: $t('modbus.sourceData'), value: 'data' },
  { name: $t('modbus.sourceScript'), value: 'script' },
];

const mapHeaders = [
  { name: $t('modbus.type'), key: 'type', className: '' },
  { name: $t('modbus.address'), key: 'address', className: '' },
  { name: $t('modbus.target'), key: 'target', className: '' },
];

const fixedMapItems = computed<IMapItem[]>(() => [
  { key: 'coil', type: $t('modbus.coil'), address: '0–5', target: $t('modbus.kcRelays'), fc: 1, start: 0, count: 6, access: 'R/W' },
  { key: 'discrete', type: $t('modbus.discrete'), address: '0–5', target: $t('modbus.kcInputs'), fc: 2, start: 0, count: 6, access: 'R' },
  { key: 'holding-0', type: $t('modbus.holding'), address: '0', target: $t('modbus.kcInputMask'), fc: 3, start: 0, count: 1, access: 'R' },
  { key: 'holding-1', type: $t('modbus.holding'), address: '1', target: $t('modbus.kcOutputMask'), fc: 3, start: 1, count: 1, access: 'R' },
  { key: 'holding-2', type: $t('modbus.holding'), address: '2–5', target: $t('modbus.kcAdc'), fc: 3, start: 2, count: 4, access: 'R' },
  { key: 'holding-6', type: $t('modbus.holding'), address: '6–7', target: $t('modbus.kcDac'), fc: 3, start: 6, count: 2, access: 'R/W' },
  { key: 'holding-8', type: $t('modbus.holding'), address: '8–10', target: $t('modbus.kcFirmware'), fc: 3, start: 8, count: 3, access: 'R' },
  { key: 'holding-11', type: $t('modbus.holding'), address: '11–12', target: $t('modbus.kcUptime'), fc: 3, start: 11, count: 2, access: 'R' },
  { key: 'holding-13', type: $t('modbus.holding'), address: '13', target: $t('modbus.kcAddress'), fc: 3, start: 13, count: 1, access: 'R' },
  { key: 'holding-14', type: $t('modbus.holding'), address: '14', target: $t('modbus.kcBaud'), fc: 3, start: 14, count: 1, access: 'R' },
  { key: 'holding-15', type: $t('modbus.holding'), address: '15', target: $t('modbus.kcRole'), fc: 3, start: 15, count: 1, access: 'R' },
  { key: 'holding-16', type: $t('modbus.holding'), address: '16', target: $t('modbus.kcLastError'), fc: 3, start: 16, count: 1, access: 'R' },
  { key: 'holding-17', type: $t('modbus.holding'), address: '17–18', target: $t('modbus.kcRequests'), fc: 3, start: 17, count: 2, access: 'R' },
  { key: 'holding-19', type: $t('modbus.holding'), address: '19–20', target: $t('modbus.kcRtc'), fc: 3, start: 19, count: 2, access: 'R' },
  { key: 'holding-21', type: $t('modbus.holding'), address: '21', target: $t('modbus.kcRawInput'), fc: 3, start: 21, count: 1, access: 'R' },
  { key: 'holding-22', type: $t('modbus.holding'), address: '22', target: $t('modbus.kcRawOutput'), fc: 3, start: 22, count: 1, access: 'R' },
]);

let timer: ReturnType<typeof setInterval> | null = null;

const form = reactive({ role: 0, address: 1, baud: 9600, parity: 0, stopBits: 1 });
const dirty = ref(false);
let profileLoaded = false;
let modbusLoaded = false;

const fileInput = ref<HTMLInputElement | null>(null);
const profileDirty = ref(false);
const profileLoading = ref(true);
const profileSending = ref<TProfileAction | null>(null);
const profileMessage = ref('');
const profileError = ref('');
const draft = ref<IModbusProfile>(emptyProfile());
const writeDrafts = reactive<Record<number, string>>({});

let profileAckTimer: ReturnType<typeof setTimeout> | null = null;
let profileSentSnapshot = '';

const deviceEdit = ref<number | null>(null);
const deviceForm = reactive<IDeviceForm>(emptyDevice(0));
const deviceErrors = reactive<Record<string, string>>({});
const pointEdit = ref<number | null>(null);
const pointForm = reactive<IPointForm>(emptyPoint(0));
const pointErrors = reactive<Record<string, string>>({});
const slaveEdit = ref<number | null>(null);
const slaveAdvanced = ref(false);
const sourceTreeOpen = ref(false);
const slaveForm = reactive<ISlaveForm>({
  sourceKind: 'data',
  sourcePath: '',
  id: '',
  name: '',
  address: SLAVE_ADDR_START,
  type: TYPE_U16,
  scale: 1,
  offset: 0,
  decimals: 0,
  unit: '',
  writable: false,
});
const slaveErrors = reactive<Record<string, string>>({});
const varEdit = ref<number | null>(null);
const varForm = reactive<IVarForm>(emptyVar(0));
const varErrors = reactive<Record<string, string>>({});

const deleteTarget = ref<IDeleteTarget | null>(null);

const pointTab = ref(0);
const pointTabs = [{ title: $t('modbus.tabSetup') }, { title: $t('modbus.tabTest') }];

const testAutoCrc = ref(true);
const testFrame = ref('');
const testPending = ref(false);
const testResult = ref<IModbusRaw | null>(null);
const testError = ref('');
const testSyncing = ref(false);
const testResultRef = ref<HTMLElement | null>(null);
let testTimer: ReturnType<typeof setTimeout> | null = null;

function emptyDevice(index: number): IDeviceForm {
  return { id: `dev${index + 1}`, name: '', address: index + 1, enabled: 1, interval: 1000, timeout: 500, retries: 1 };
}

function emptyPoint(index: number): IPointForm {
  return {
    device: 0,
    id: `p${index + 1}`,
    name: '',
    fc: 3,
    address: 0,
    type: TYPE_U16,
    flags: 0,
    registers: 1,
    decimals: 0,
    scale: 1,
    offset: 0,
    unit: '',
    var: NONE,
  };
}

function emptyVar(index: number): IVarForm {
  return { id: `v${index + 1}`, name: '', type: TYPE_U16, flags: 0, unit: '', decimals: 0, point: index < 64 ? index : NONE, source: '' };
}

function makeDevice(): IModbusDevice {
  return { id: '', name: '', address: 1, enabled: 1, interval: 1000, timeout: 500, retries: 1 };
}

function makePoint(): IModbusPoint {
  return {
    device: 0,
    id: '',
    name: '',
    fc: 3,
    address: 0,
    type: TYPE_U16,
    flags: 0,
    registers: 1,
    decimals: 0,
    scale: 1,
    offset: 0,
    unit: '',
    var: NONE,
  };
}

function makeVar(): IModbusVar {
  return { id: '', name: '', type: TYPE_U16, flags: 0, unit: '', decimals: 0, point: NONE, source: '' };
}

function emptyProfile(): IModbusProfile {
  return {
    command: 0,
    version: 2,
    deviceCount: 0,
    pointCount: 0,
    varCount: 0,
    reserved: 0,
    devices: Array.from({ length: 8 }, makeDevice),
    points: Array.from({ length: 64 }, makePoint),
    vars: Array.from({ length: 32 }, makeVar),
  };
}

function normalizeProfile(value: IModbusProfile): IModbusProfile {
  const pick = <T extends object>(source: T | undefined, fallback: T): T => ({ ...fallback, ...(source ?? {}) });
  const devices = Array.from({ length: 8 }, (_, i) => pick(value.devices?.[i], makeDevice()));
  const points = Array.from({ length: 64 }, (_, i) => pick(value.points?.[i], makePoint()));
  const vars = Array.from({ length: 32 }, (_, i) => pick(value.vars?.[i], makeVar()));

  // Older profiles could carry a smaller count than the selected type needs.
  for (const point of points) {
    if (point.type !== TYPE_STRING) point.registers = Math.max(point.registers || 1, registersForType(point.type, 1));
  }

  return {
    command: 0,
    version: value.version || 2,
    deviceCount: Math.min(8, value.deviceCount || 0),
    pointCount: Math.min(64, value.pointCount || 0),
    varCount: Math.min(32, value.varCount || 0),
    reserved: 0,
    devices,
    points,
    vars,
  };
}

const stopTimer = () => {
  if (timer) clearInterval(timer);
  timer = null;
};

const clearProfileAck = () => {
  if (profileAckTimer) clearTimeout(profileAckTimer);
  profileAckTimer = null;
};

const completeProfileSend = (incoming: IModbusProfile) => {
  clearProfileAck();
  const action = profileSending.value;
  profileSending.value = null;

  if (!action) return;

  // The firmware silently keeps the old profile when validation fails. Compare
  // the reply with the snapshot we sent so the UI does not report a false save.
  if (JSON.stringify(incoming) === profileSentSnapshot) {
    draft.value = incoming;
    profileDirty.value = false;
    profileMessage.value = action === 'apply' ? $t('modbus.profileApplied') : $t('modbus.profileSaved');
    profileError.value = '';
  } else {
    profileError.value = $t('modbus.profileRejected');
  }
};

const onMessage = ({ key, object }: TMessage) => {
  if (key === KEY.MODBUSRAW) {
    if (!testPending.value || !object) return;

    const value = object as IModbusRaw;
    if (value.status === 1) return;

    if (testTimer) clearTimeout(testTimer);
    testTimer = null;
    testPending.value = false;
    testResult.value = value;
    nextTick(() => testResultRef.value?.scrollIntoView({ block: 'nearest' }));
    return;
  }

  if (key === KEY.MODBUSPROFILE) {
    profileLoaded = true;
    profileLoading.value = false;
    if (modbusLoaded) stopTimer();
    if (!object) return;

    const incoming = normalizeProfile(object as unknown as IModbusProfile);
    if (profileSending.value) {
      completeProfileSend(incoming);
      return;
    }

    if (!profileDirty.value) draft.value = incoming;
    return;
  }
  if (key !== KEY.MODBUS) return;

  modbusLoaded = true;
  if (profileLoaded) stopTimer();

  if (!dirty.value) return;
  if (!object) return;
  const value = object as IModbus;
  if (value.role === form.role && value.address === form.address && value.baud === form.baud && value.parity === form.parity && value.stopBits === form.stopBits) {
    dirty.value = false;
  }
};

const { main, onSend } = useConnection((send) => {
  send(KEY.MODBUS, { command: CMD.GET });
  send(KEY.MODBUSPROFILE, { command: CMD.GET });
  send(KEY.MODBUSRAW, { command: CMD.GET });
  send(KEY.PORT, { gpio: 0, command: CMD.GET_ALL });

  if (timer) return;
  let attempts = 0;
  timer = setInterval(() => {
    attempts += 1;
    if (attempts > 5) {
      profileLoading.value = false;
      if (!profileLoaded) profileError.value = $t('modbus.profileNoResponse');
      return stopTimer();
    }
    send(KEY.MODBUS, { command: CMD.GET });
    send(KEY.MODBUSPROFILE, { command: CMD.GET });
    send(KEY.MODBUSRAW, { command: CMD.GET });
  }, 2000);
}, onMessage);

onUnmounted(() => {
  stopTimer();
  clearProfileAck();
  if (testTimer) clearTimeout(testTimer);
});

const status = computed(() => main.value.modbus as IModbus);

// Пока пользователь редактирует, live-кадры не должны затирать draft.
watch(
  status,
  (value) => {
    if (dirty.value) return;
    form.role = value.role;
    form.address = value.address;
    form.baud = value.baud;
    form.parity = value.parity;
    form.stopBits = value.stopBits;
  },
  { immediate: true }
);
const roleName = computed(() => roles.find((role) => role.value === form.role)?.name ?? $t('modbus.off'));
const baudName = computed(() => String(form.baud));
const parityName = computed(() => parities.find((item) => item.value === form.parity)?.name ?? $t('modbus.parityNone'));
const stopBitsName = computed(() => String(form.stopBits));

const formatText = computed(() => {
  const parity = form.parity === 1 ? 'E' : form.parity === 2 ? 'O' : 'N';
  return `8${parity}${form.stopBits}`;
});

const statusText = computed(() => {
  if (status.value.role === 0) return $t('modbus.off');
  if (status.value.role === 2) return status.value.status === 1 ? $t('modbus.polling') : $t('modbus.error');
  if (status.value.status === 1) return $t('modbus.listening');
  return $t('modbus.error');
});

const lastErrorText = computed(() => {
  const errors: Record<number, string> = {
    0: $t('modbus.noError'),
    1: $t('modbus.crc'),
    2: $t('modbus.addressError'),
    3: $t('modbus.functionError'),
    4: $t('modbus.rangeError'),
    5: $t('modbus.frameError'),
  };
  return errors[status.value.lastError] ?? $t('modbus.noError');
});

const statusItems = computed(() => {
  const online = status.value.status === 1;
  return [
    { label: $t('modbus.state'), value: statusText.value, className: online ? 'text-green-600 dark:text-green-400' : 'text-red-600 dark:text-red-400' },
    {
      label: $t('modbus.lastError'),
      value: status.value.lastError ? `${lastErrorText.value} (${status.value.lastError})` : lastErrorText.value,
      className: status.value.lastError ? 'text-red-600 dark:text-red-400' : '',
    },
    { label: $t('modbus.requests'), value: String(status.value.requests), className: '' },
    { label: $t('modbus.errors'), value: String(status.value.errors), className: status.value.errors ? 'text-amber-600 dark:text-amber-400' : '' },
    { label: $t('modbus.lastFunction'), value: status.value.lastFunction ? `0x${status.value.lastFunction.toString(16).toUpperCase()}` : '—', className: '' },
    { label: $t('modbus.lastAddress'), value: status.value.lastAddress ? String(status.value.lastAddress) : '—', className: '' },
  ];
});

const clamp = (value: string | number, min: number, max: number) => {
  const parsed = typeof value === 'number' ? value : Number(String(value).replace(',', '.'));
  if (!Number.isFinite(parsed)) return min;
  return Math.min(max, Math.max(min, Math.trunc(parsed)));
};

// Editors accept out-of-range numbers and explain them inline instead of
// silently clamping the value under the cursor.
const toInt = (value: string | number, fallback = 0) => {
  const parsed = typeof value === 'number' ? value : Number(String(value).replace(',', '.'));
  return Number.isFinite(parsed) ? Math.trunc(parsed) : fallback;
};

const clearErrors = (errors: Record<string, string>) => {
  for (const key of Object.keys(errors)) delete errors[key];
};

const duplicateId = (list: Array<{ id: string }>, count: number, skip: number, id: string) => {
  const value = id.trim();
  if (!value) return false;
  for (let i = 0; i < count; i++) {
    if (i !== skip && list[i]?.id === value) return true;
  }
  return false;
};

const toNumber = (value: string, fallback: number) => {
  const parsed = Number(String(value).replace(',', '.'));
  return Number.isFinite(parsed) ? parsed : fallback;
};

const selectName = (items: IListItem<number>[], value: number) => items.find((item) => item.value === value)?.name ?? '';

const typeName = (value: number) => typeItems.find((item) => item.value === value)?.name ?? String(value);
const deviceName = (index: number) => {
  if (index === DEVICE_SLAVE) return $t('modbus.slaveValue');
  if (index === NONE || index < 0) return '—';
  const device = draft.value.devices[index];
  return device?.name || device?.id || `#${index + 1}`;
};

const nextSlaveAddress = (skip: number) => {
  let address = SLAVE_ADDR_START;
  while (address + 1 <= SLAVE_ADDR_START + SLAVE_REG_COUNT) {
    const busy = draft.value.points.some((point, index) => {
      if (index === skip || point.device !== DEVICE_SLAVE) return false;
      const width = Math.max(1, point.registers || registersForType(point.type, 1));
      return address >= point.address && address < point.address + width;
    });
    if (!busy) return address;
    address += 1;
  }
  return SLAVE_ADDR_START;
};

const pointEntries = computed(() => draft.value.points.slice(0, draft.value.pointCount).map((point, index) => ({ point, index })));
const masterPointEntries = computed(() => pointEntries.value.filter(({ point }) => point.device !== DEVICE_SLAVE));
const slavePointEntries = computed(() => pointEntries.value.filter(({ point }) => point.device === DEVICE_SLAVE));
const slaveBankUsed = computed(() => slavePointEntries.value.reduce((sum, { point }) => sum + Math.max(1, point.registers || registersForType(point.type, 1)), 0));
const nextSlaveVarId = computed(() => uniqueId('v', draft.value.vars, draft.value.varCount));

const uniqueId = (prefix: string, items: { id: string }[], count: number) => {
  let n = count + 1;
  while (items.slice(0, count).some((item) => item.id === `${prefix}${n}`)) n += 1;
  return `${prefix}${n}`;
};

const pointName = (index: number) => {
  if (index === NONE || index < 0) return '—';
  const point = draft.value.points[index];
  return point?.name || point?.id || `#${index + 1}`;
};
const varName = (index: number) => {
  if (index === NONE || index < 0) return '—';
  const variable = draft.value.vars[index];
  return variable?.name || variable?.id || `#${index + 1}`;
};

const pointDeviceItems = computed<IListItem<number>[]>(() =>
  draft.value.devices.slice(0, draft.value.deviceCount).map((device, index) => ({
    name: device.name || device.id || `#${index + 1}`,
    value: index,
  }))
);

const pointVarItems = computed<IListItem<number>[]>(() => [
  { name: '—', value: NONE },
  ...draft.value.vars.slice(0, draft.value.varCount).map((variable, index) => ({
    name: variable.name || variable.id || `#${index + 1}`,
    value: index,
  })),
]);

const masterVarItems = computed<IListItem<number>[]>(() => pointVarItems.value.filter(({ value }) => value === NONE || !draft.value.vars[value]?.source));

const varPointItems = computed<IListItem<number>[]>(() => [
  { name: '—', value: NONE },
  ...draft.value.points
    .slice(0, draft.value.pointCount)
    .map((point, index) => ({ point, index }))
    .filter(({ point }) => point.device !== DEVICE_SLAVE)
    .map(({ point, index }) => ({
      name: point.name || point.id || `#${index + 1}`,
      value: index,
    })),
]);

const profileErrors = computed<string[]>(() => {
  const errors: string[] = [];
  const ids = new Set<string>();

  if (draft.value.deviceCount > 8) errors.push($t('modbus.limitDevices'));

  for (let i = 0; i < draft.value.deviceCount; i++) {
    const device = draft.value.devices[i];
    if (!device?.id) errors.push(`${$t('modbus.device')} #${i + 1}: ${$t('modbus.id')}`);
    else if (ids.has(device.id)) errors.push(`${$t('modbus.duplicateId')}: ${device.id}`);
    else ids.add(device.id);

    if (device && (device.address < 1 || device.address > 247)) errors.push(`${$t('modbus.device')} #${i + 1}: ${$t('modbus.address')}`);
    if (device && device.interval < 100) errors.push(`${$t('modbus.device')} #${i + 1}: ${$t('modbus.interval')}`);
    if (device && device.timeout < 50) errors.push(`${$t('modbus.device')} #${i + 1}: ${$t('modbus.timeout')}`);
    if (device && device.retries > 5) errors.push(`${$t('modbus.device')} #${i + 1}: ${$t('modbus.retries')}`);
  }

  if (draft.value.pointCount > 64) errors.push($t('modbus.limitPoints'));

  ids.clear();
  for (let i = 0; i < draft.value.pointCount; i++) {
    const point = draft.value.points[i];
    if (!point?.id) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.id')}`);
    else if (ids.has(point.id)) errors.push(`${$t('modbus.duplicateId')}: ${point.id}`);
    else ids.add(point.id);

    if (!point) continue;

    if (point.device === DEVICE_SLAVE) {
      if (point.fc !== 3 && point.fc !== 4) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.fc')}`);
      if (point.var === NONE || point.var >= draft.value.varCount) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.sourceVar')}`);
      const width = Math.max(1, point.registers || registersForType(point.type, 1));
      if (point.address < SLAVE_ADDR_START || point.address + width - 1 >= SLAVE_ADDR_START + SLAVE_REG_COUNT) {
        errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.address')}`);
      }
    } else {
      if (point.device >= draft.value.deviceCount) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.device')}`);
      if (point.fc < 1 || point.fc > 4) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.fc')}`);
      if (point.var !== NONE && point.var >= draft.value.varCount) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.variable')}`);
      else if (point.var !== NONE && draft.value.vars[point.var]?.source) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.sourceVar')}`);
    }

    if (point.address < 0 || point.address > 65535) errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.address')}`);
  }

  for (let i = 0; i < draft.value.pointCount; i++) {
    const a = draft.value.points[i];
    if (a?.device !== DEVICE_SLAVE) continue;
    const widthA = Math.max(1, a.registers || registersForType(a.type, 1));
    for (let j = i + 1; j < draft.value.pointCount; j++) {
      const b = draft.value.points[j];
      if (b?.device !== DEVICE_SLAVE) continue;
      const widthB = Math.max(1, b.registers || registersForType(b.type, 1));
      if (a.address < b.address + widthB && b.address < a.address + widthA) {
        errors.push(`${$t('modbus.point')} #${i + 1}: ${$t('modbus.addressTaken')}`);
      }
    }
  }

  const sourceVars = new Set<number>();
  for (let i = 0; i < draft.value.pointCount; i++) {
    const point = draft.value.points[i];
    if (!point || point.var === NONE || point.device === DEVICE_SLAVE) continue;
    if (sourceVars.has(point.var)) errors.push(`${$t('modbus.vars')} #${point.var + 1}: ${$t('modbus.sourceVar')}`);
    else sourceVars.add(point.var);
  }

  if (draft.value.varCount > 32) errors.push($t('modbus.limitVars'));

  ids.clear();
  for (let i = 0; i < draft.value.varCount; i++) {
    const variable = draft.value.vars[i];
    if (!variable?.id) errors.push(`${$t('modbus.vars')} #${i + 1}: ${$t('modbus.id')}`);
    else if (ids.has(variable.id)) errors.push(`${$t('modbus.duplicateId')}: ${variable.id}`);
    else ids.add(variable.id);

    if (variable && variable.point !== NONE && variable.point >= draft.value.pointCount) {
      errors.push(`${$t('modbus.vars')} #${i + 1}: ${$t('modbus.point')}`);
    }
  }

  return errors;
});

const profileValid = computed(() => profileErrors.value.length === 0);
const profileErrorText = computed(() => profileError.value || profileErrors.value[0] || '');
const profileSendingText = computed(() => (profileSending.value === 'save' ? $t('modbus.profileSaving') : $t('modbus.profileApplying')));
const deleteHint = computed(() => (deleteTarget.value ? $t('modbus.deleteHint', { name: deleteTarget.value.name }) : ''));

const appliedVarCount = computed(() => main.value.modbus.varCount ?? 0);

const liveOf = (point: IModbusPoint) => {
  if (!point || point.var === NONE || point.var >= appliedVarCount.value) return undefined;
  return main.value.modbus.vars[point.var];
};

const pointValue = (point: IModbusPoint) => {
  const live = liveOf(point);
  if (!live) return '—';
  if (live.quality !== 0) return '—';
  if (point.type === TYPE_STRING) return live.text || '—';

  const value = Number(live.value).toFixed(point.decimals || 0);
  return point.unit ? `${value} ${point.unit}` : value;
};

const varValue = (variable: IModbusVar, index: number) => {
  if (index >= appliedVarCount.value) return '—';
  const live = main.value.modbus.vars[index];
  if (!live || live.quality !== 0) return '—';
  if (variable.type === TYPE_STRING) return live.text || '—';
  const value = Number(live.value).toFixed(variable.decimals || 0);
  return variable.unit ? `${value} ${variable.unit}` : value;
};

const qualityName = (quality?: number) => {
  if (quality === 0) return $t('modbus.qOk');
  if (quality === 1 || quality === undefined) return $t('modbus.qStale');
  if (quality === 3) return $t('modbus.qDisabled');
  return $t('modbus.qError');
};

const qualityClass = (quality?: number) => {
  if (quality === 0) return 'bg-green-500/15 text-green-700 dark:text-green-300';
  if (quality === 1 || quality === undefined) return 'bg-gray-500/15 text-gray-700 dark:text-gray-300';
  if (quality === 3) return 'bg-gray-500/15 text-gray-700 dark:text-gray-300';
  return 'bg-red-500/15 text-red-700 dark:text-red-300';
};

const isDesktop = useMediaQuery('(min-width: 1024px)');
const mapDetails = ref<IMapDetails | null>(null);

const isPointApplied = (index: number) => {
  const applied = main.value.modbusProfile?.points?.[index];
  const point = draft.value.points[index];
  if (!applied || !point) return false;
  if (
    applied.device !== point.device ||
    applied.fc !== point.fc ||
    applied.address !== point.address ||
    applied.type !== point.type ||
    applied.flags !== point.flags ||
    applied.registers !== point.registers ||
    applied.decimals !== point.decimals ||
    applied.scale !== point.scale ||
    applied.offset !== point.offset ||
    applied.unit !== point.unit ||
    applied.var !== point.var
  ) {
    return false;
  }
  if (point.var === NONE) return true;
  const variable = draft.value.vars[point.var];
  const appliedVar = main.value.modbusProfile?.vars?.[point.var];
  if (!variable || !appliedVar) return false;
  return (
    appliedVar.id === variable.id &&
    appliedVar.name === variable.name &&
    appliedVar.type === variable.type &&
    appliedVar.unit === variable.unit &&
    appliedVar.decimals === variable.decimals &&
    appliedVar.source === variable.source
  );
};

const slaveMapRows = computed<IMapItem[]>(() => {
  return slavePointEntries.value
    .map(({ point, index }) => {
      const width = Math.max(1, point.registers || registersForType(point.type, 1));
      const variable = point.var !== NONE ? draft.value.vars[point.var] : undefined;
      const name = point.name || variable?.name || point.id || `#${index + 1}`;
      return {
        key: `slave-${index}`,
        type: $t('modbus.holding'),
        address: `${point.address}–${point.address + width - 1}`,
        target: `${name} · ${typeName(point.type)}`,
        fc: 3,
        start: point.address,
        count: width,
        access: point.flags & 4 ? 'R/W' : 'R',
        child: true,
        pointIndex: index,
        notApplied: !isPointApplied(index),
      };
    })
    .sort((a, b) => a.start - b.start);
});

const mapItems = computed<IMapItem[]>(() => [...fixedMapItems.value, ...slaveMapRows.value]);

const countLabel = (fc: number) => (fc <= 2 ? $t('modbus.bits') : $t('modbus.registers'));

const formatNote = (point: IModbusPoint) => {
  const parts = [typeName(point.type)];
  if (point.flags & 2) parts.push($t('modbus.byteLittle'));
  if (point.flags & 1) parts.push($t('modbus.wordLittle'));
  if (point.scale !== 1) parts.push(`${$t('modbus.scale')} ${point.scale}`);
  if (point.offset !== 0) parts.push(`${$t('modbus.offset')} ${point.offset}`);
  return parts.join(' · ');
};

const buildMapDetails = (item: IMapItem): IMapDetails => {
  const readFrame = formatHex(buildReadFrame(form.address, item.fc, item.start, item.count));

  if (item.child && item.pointIndex !== undefined) {
    const point = draft.value.points[item.pointIndex];
    const variable = point.var !== NONE ? draft.value.vars[point.var] : undefined;
    const live = liveOf(point);
    const bytes = live && live.quality === 0 ? encodePointValue(point, live.value) : undefined;

    return {
      title: point.name || variable?.name || point.id || item.target,
      subtitle: variable?.source || $t('modbus.sourceScript'),
      type: typeName(point.type),
      address: item.address,
      fc: item.fc,
      count: item.count,
      countLabel: countLabel(item.fc),
      access: item.access,
      hex: bytes ? formatHex(bytes) : '—',
      source: variable?.source || $t('modbus.sourceScript'),
      live: pointValue(point),
      readFrame,
      responseFrame: bytes ? formatHex(buildReadResponse(form.address, item.fc, bytes)) : undefined,
      note: formatNote(point),
      notApplied: item.notApplied,
    };
  }

  return {
    title: item.target,
    type: item.type,
    address: item.address,
    fc: item.fc,
    count: item.count,
    countLabel: countLabel(item.fc),
    access: item.access,
    hex: '—',
    readFrame,
    note: $t('modbus.systemRegisterNote'),
  };
};

const onMapClick = (item: IMapItem) => {
  mapDetails.value = buildMapDetails(item);
};

const closeMapDetails = () => {
  mapDetails.value = null;
};

const markDirty = () => {
  profileDirty.value = true;
  profileMessage.value = '';
  profileError.value = '';
};

const setFlag = (target: { flags: number }, flag: number, value: boolean) => {
  target.flags = value ? target.flags | flag : target.flags & ~flag;
};

const onPointType = (value: number) => {
  pointForm.type = value;
  pointForm.registers = registersForType(value, pointForm.registers);
  // Float values are unreadable with zero decimals; give them a sane default.
  if ((value === TYPE_FLOAT32 || value === TYPE_FLOAT64) && pointForm.decimals === 0) pointForm.decimals = 2;
};

const onVarType = (value: number) => {
  varForm.type = value;
  if ((value === TYPE_FLOAT32 || value === TYPE_FLOAT64) && varForm.decimals === 0) varForm.decimals = 2;
};

const rangeError = () => $t('modbus.outOfRange');
const tooLong = (max: number) => $t('modbus.maxLength', { max });

const validateDeviceForm = () => {
  clearErrors(deviceErrors);
  let ok = true;
  const id = deviceForm.id.trim();

  if (!id) {
    deviceErrors.id = $t('modbus.required');
    ok = false;
  } else if (id.length > 11) {
    deviceErrors.id = tooLong(11);
    ok = false;
  } else if (duplicateId(draft.value.devices, draft.value.deviceCount, deviceEdit.value ?? -1, id)) {
    deviceErrors.id = $t('modbus.duplicateId');
    ok = false;
  }

  if (deviceForm.name.length > 19) {
    deviceErrors.name = tooLong(19);
    ok = false;
  }

  if (deviceForm.address < 1 || deviceForm.address > 247) {
    deviceErrors.address = rangeError();
    ok = false;
  }
  if (deviceForm.interval < 100 || deviceForm.interval > 600000) {
    deviceErrors.interval = rangeError();
    ok = false;
  }
  if (deviceForm.timeout < 50 || deviceForm.timeout > 10000) {
    deviceErrors.timeout = rangeError();
    ok = false;
  }
  if (deviceForm.retries < 0 || deviceForm.retries > 5) {
    deviceErrors.retries = rangeError();
    ok = false;
  }

  return ok;
};

const validatePointForm = () => {
  clearErrors(pointErrors);
  let ok = true;
  const id = pointForm.id.trim();

  if (!id) {
    pointErrors.id = $t('modbus.required');
    ok = false;
  } else if (id.length > 11) {
    pointErrors.id = tooLong(11);
    ok = false;
  } else if (duplicateId(draft.value.points, draft.value.pointCount, pointEdit.value ?? -1, id)) {
    pointErrors.id = $t('modbus.duplicateId');
    ok = false;
  }

  if (pointForm.name.length > 19) {
    pointErrors.name = tooLong(19);
    ok = false;
  }
  if (pointForm.unit.length > 7) {
    pointErrors.unit = tooLong(7);
    ok = false;
  }

  if (pointForm.device < 0 || pointForm.device >= draft.value.deviceCount) {
    pointErrors.device = rangeError();
    ok = false;
  }
  if (pointForm.fc < 1 || pointForm.fc > 4) {
    pointErrors.device = $t('modbus.outOfRange');
    ok = false;
  }
  if (pointForm.var !== NONE && (pointForm.var < 0 || pointForm.var >= draft.value.varCount)) {
    pointErrors.var = rangeError();
    ok = false;
  } else if (pointForm.var !== NONE && draft.value.vars[pointForm.var]?.source) {
    pointErrors.var = $t('modbus.sourceVar');
    ok = false;
  }

  if (pointForm.address < 0 || pointForm.address > 65535) {
    pointErrors.address = rangeError();
    ok = false;
  }
  if (pointForm.registers < 1 || pointForm.registers > 16) {
    pointErrors.registers = rangeError();
    ok = false;
  }
  if (pointForm.decimals < 0 || pointForm.decimals > 6) {
    pointErrors.decimals = rangeError();
    ok = false;
  }

  return ok;
};

const validateVarForm = () => {
  clearErrors(varErrors);
  let ok = true;
  const id = varForm.id.trim();

  if (!id) {
    varErrors.id = $t('modbus.required');
    ok = false;
  } else if (id.length > 15) {
    varErrors.id = tooLong(15);
    ok = false;
  } else if (duplicateId(draft.value.vars, draft.value.varCount, varEdit.value ?? -1, id)) {
    varErrors.id = $t('modbus.duplicateId');
    ok = false;
  }

  if (varForm.name.length > 19) {
    varErrors.name = tooLong(19);
    ok = false;
  }
  if (varForm.unit.length > 7) {
    varErrors.unit = tooLong(7);
    ok = false;
  }

  if (varForm.point !== NONE && (varForm.point < 0 || varForm.point >= draft.value.pointCount)) {
    varErrors.point = rangeError();
    ok = false;
  }
  if (varForm.decimals < 0 || varForm.decimals > 6) {
    varErrors.decimals = rangeError();
    ok = false;
  }

  return ok;
};

const openDevice = (index?: number) => {
  clearErrors(deviceErrors);
  if (index === undefined) {
    if (draft.value.deviceCount >= 8) return;
    Object.assign(deviceForm, emptyDevice(draft.value.deviceCount));
    deviceEdit.value = -1;
  } else {
    Object.assign(deviceForm, draft.value.devices[index]);
    deviceEdit.value = index;
  }
};

const saveDevice = () => {
  if (deviceEdit.value === null) return;
  if (!validateDeviceForm()) return;

  if (deviceEdit.value < 0) {
    if (draft.value.deviceCount >= 8) return;
    draft.value.devices[draft.value.deviceCount] = { ...deviceForm };
    draft.value.deviceCount += 1;
  } else {
    draft.value.devices[deviceEdit.value] = { ...deviceForm };
  }

  deviceEdit.value = null;
  markDirty();
};

const askRemoveDevice = (index: number) => {
  for (let i = 0; i < draft.value.pointCount; i++) {
    if (draft.value.points[i].device === index) {
      profileError.value = $t('modbus.deviceInUse');
      return;
    }
  }

  deleteTarget.value = { kind: 'device', index, name: deviceName(index) };
};

const performRemoveDevice = (index: number) => {
  draft.value.devices.splice(index, 1);
  draft.value.devices.push(makeDevice());
  draft.value.deviceCount -= 1;

  for (let i = 0; i < draft.value.pointCount; i++) {
    const point = draft.value.points[i];
    if (point.device !== DEVICE_SLAVE && point.device > index) point.device -= 1;
  }

  deviceEdit.value = null;
  markDirty();
};

const sourceTree = computed(() => ({
  ports: main.value.ports,
  device: main.value.device,
  dallas: main.value.dallas,
  info: main.value.info,
  modbus: main.value.modbus,
}));

const sourceValue = (path: string): unknown => {
  let node: unknown = sourceTree.value;
  for (const part of path.split('.')) {
    if (node === null || typeof node !== 'object' || Array.isArray(node)) return undefined;
    node = (node as Record<string, unknown>)[part];
  }
  return node;
};

const canAddSource = (path: string) => {
  if (!path || path.length >= 32) return false;
  const parts = path.split('.');
  const meta = parts[0] === 'ports' || parts[0] === 'dallas' ? SOURCE_FIELDS[parts[0]]?.[parts[2]] : SOURCE_FIELDS[parts[0]]?.[parts[1]];
  if (!meta) return false;
  const value = sourceValue(path);
  return typeof value === 'number' && Number.isFinite(value);
};

const sourceMeta = (path: string) => {
  const parts = path.split('.');
  if (parts[0] === 'ports') {
    const meta = SOURCE_FIELDS.ports[parts[2]];
    if (!meta) return null;
    const name = parts[2] === 'value' ? `GPIO ${parts[1]}` : `GPIO ${parts[1]} ${parts[2]}`;
    return { ...meta, name };
  }
  if (parts[0] === 'dallas') {
    const meta = SOURCE_FIELDS.dallas[parts[2]];
    if (!meta) return null;
    const index = Object.keys(main.value.dallas).sort().indexOf(parts[1]);
    return { ...meta, name: `Dallas ${index + 1}` };
  }
  const meta = SOURCE_FIELDS[parts[0]]?.[parts[1]];
  return meta ? { ...meta, name: parts[1] } : null;
};

const varSourceLabel = (variable?: IModbusVar) => {
  if (!variable) return '—';
  if (variable.source) return variable.source;
  if (variable.point !== NONE) return pointName(variable.point);
  return $t('modbus.sourceScript');
};

const slaveSourceKindName = computed(() => slaveSourceKindItems.find((item) => item.value === slaveForm.sourceKind)?.name ?? '');
const slaveTypeText = computed(() => typeName(slaveForm.type));
const slaveWritableHint = computed(() => slaveForm.sourceKind === 'data' && Boolean(slaveForm.sourcePath) && !(slaveForm.sourcePath.startsWith('ports.') && slaveForm.sourcePath.endsWith('.value')));

const openSourceTree = () => {
  sourceTreeOpen.value = true;
};

const closeSourceTree = () => {
  sourceTreeOpen.value = false;
};

const onSourcePath = ({ path, isObject }: { path: string; isObject: boolean }) => {
  if (isObject || !canAddSource(path)) return;
  const meta = sourceMeta(path);
  if (!meta) return;
  slaveForm.sourceKind = 'data';
  slaveForm.sourcePath = path;
  slaveForm.type = meta.type;
  slaveForm.decimals = meta.decimals ?? 0;
  slaveForm.unit = meta.unit ?? '';
  if (!slaveForm.name) slaveForm.name = meta.name;
  sourceTreeOpen.value = false;
};

const onSlaveSourceKind = (value: string) => {
  const kind = value as TSlaveSourceKind;
  slaveForm.sourceKind = kind;
  clearErrors(slaveErrors);
  if (kind === 'script') {
    slaveForm.sourcePath = '';
    slaveForm.type = TYPE_U16;
    slaveForm.decimals = 0;
    slaveForm.unit = '';
    if (!slaveForm.name) slaveForm.name = $t('modbus.sourceScript');
  } else if (slaveForm.name === $t('modbus.sourceScript')) {
    slaveForm.name = '';
  }
};

const slaveTarget = () => {
  const isEdit = slaveEdit.value !== null && slaveEdit.value >= 0;
  const pointIndex = isEdit ? (slaveEdit.value as number) : draft.value.pointCount;
  let varIndex = draft.value.varCount;
  let needsNewVar = true;

  if (isEdit) {
    const point = draft.value.points[pointIndex];
    const current = point?.var ?? NONE;
    if (current !== NONE && current < draft.value.varCount) {
      const shared = draft.value.points.some((item, index) => index < draft.value.pointCount && index !== pointIndex && item.var === current);
      if (!shared) {
        varIndex = current;
        needsNewVar = false;
      }
    }
  }

  return { isEdit, pointIndex, varIndex, needsNewVar, needsNewPoint: !isEdit };
};

const openSlaveValue = () => {
  clearErrors(slaveErrors);
  Object.assign(slaveForm, {
    sourceKind: 'data',
    sourcePath: '',
    id: nextSlaveVarId.value,
    name: '',
    address: nextSlaveAddress(-1),
    type: TYPE_U16,
    scale: 1,
    offset: 0,
    decimals: 0,
    unit: '',
    writable: false,
  });
  slaveAdvanced.value = false;
  slaveEdit.value = -1;
};

const openSlaveValueEdit = (index: number) => {
  const point = draft.value.points[index];
  if (!point || point.device !== DEVICE_SLAVE) return;
  clearErrors(slaveErrors);
  const variable = point.var !== NONE ? draft.value.vars[point.var] : undefined;
  Object.assign(slaveForm, {
    sourceKind: variable?.source ? 'data' : 'script',
    sourcePath: variable?.source ?? '',
    id: variable?.id || nextSlaveVarId.value,
    name: point.name || variable?.name || '',
    address: point.address,
    type: point.type,
    scale: point.scale,
    offset: point.offset,
    decimals: point.decimals,
    unit: point.unit,
    writable: Boolean(point.flags & 4),
  });
  slaveAdvanced.value = false;
  slaveEdit.value = index;
};

const closeSlaveValue = () => {
  slaveEdit.value = null;
};

const validateSlaveForm = () => {
  clearErrors(slaveErrors);
  let ok = true;
  const target = slaveTarget();

  if (slaveForm.sourceKind === 'data' && (!slaveForm.sourcePath || !canAddSource(slaveForm.sourcePath))) {
    slaveErrors.source = $t('modbus.sourceInvalid');
    ok = false;
  }

  const id = slaveForm.id.trim();
  if (!id) {
    slaveErrors.id = $t('modbus.required');
    ok = false;
  } else if (id.length > 15) {
    slaveErrors.id = tooLong(15);
    ok = false;
  } else if (draft.value.vars.some((variable, index) => index < draft.value.varCount && index !== (target.needsNewVar ? -1 : target.varIndex) && variable.id === id)) {
    slaveErrors.id = $t('modbus.duplicateId');
    ok = false;
  }

  const name = slaveForm.name.trim();
  if (!name) {
    slaveErrors.name = $t('modbus.required');
    ok = false;
  } else if (name.length > 19) {
    slaveErrors.name = tooLong(19);
    ok = false;
  }

  if (slaveForm.unit.length > 7) {
    slaveErrors.unit = tooLong(7);
    ok = false;
  }
  if (slaveForm.decimals < 0 || slaveForm.decimals > 6) {
    slaveErrors.decimals = rangeError();
    ok = false;
  }

  const width = registersForType(slaveForm.type, 1);
  if (slaveForm.address < SLAVE_ADDR_START || slaveForm.address + width - 1 >= SLAVE_ADDR_START + SLAVE_REG_COUNT) {
    slaveErrors.address = rangeError();
    ok = false;
  } else {
    const overlap = draft.value.points.some((point, index) => {
      if (index >= draft.value.pointCount || index === target.pointIndex || point.device !== DEVICE_SLAVE) return false;
      const otherWidth = Math.max(1, point.registers || registersForType(point.type, 1));
      return slaveForm.address < point.address + otherWidth && point.address < slaveForm.address + width;
    });
    if (overlap) {
      slaveErrors.address = $t('modbus.addressTaken');
      ok = false;
    }
  }

  if (draft.value.pointCount + (target.needsNewPoint ? 1 : 0) > 64 || draft.value.varCount + (target.needsNewVar ? 1 : 0) > 32) {
    slaveErrors.source = $t('modbus.limit');
    ok = false;
  }

  return ok;
};

const saveSlaveValue = () => {
  if (!validateSlaveForm()) return;

  const target = slaveTarget();
  const width = registersForType(slaveForm.type, 1);
  const flags = slaveForm.writable ? 4 : 0;
  const name = slaveForm.name.trim();
  const source = slaveForm.sourceKind === 'data' ? slaveForm.sourcePath : '';
  const oldPoint = target.isEdit ? draft.value.points[target.pointIndex] : undefined;

  let varIndex = target.varIndex;
  if (target.needsNewVar) {
    varIndex = draft.value.varCount;
    draft.value.varCount += 1;
  }

  const variable = draft.value.vars[varIndex];
  variable.id = slaveForm.id.trim();
  variable.name = name;
  variable.type = slaveForm.type;
  variable.flags = 0;
  variable.unit = slaveForm.unit;
  variable.decimals = slaveForm.decimals;
  variable.point = NONE;
  variable.source = source;

  draft.value.points[target.pointIndex] = {
    device: DEVICE_SLAVE,
    id: oldPoint?.id || uniqueId('e', draft.value.points, draft.value.pointCount),
    name,
    fc: 3,
    address: slaveForm.address,
    type: slaveForm.type,
    flags,
    registers: width,
    decimals: slaveForm.decimals,
    scale: slaveForm.scale,
    offset: slaveForm.offset,
    unit: slaveForm.unit,
    var: varIndex,
  };

  if (target.needsNewPoint) draft.value.pointCount += 1;

  slaveEdit.value = null;
  markDirty();
};

const openPoint = (index?: number) => {
  clearErrors(pointErrors);
  if (index === undefined) {
    if (draft.value.pointCount >= 64) return;
    Object.assign(pointForm, emptyPoint(draft.value.pointCount));
    pointForm.device = Math.min(pointForm.device, Math.max(0, draft.value.deviceCount - 1));
    pointEdit.value = -1;
  } else {
    Object.assign(pointForm, draft.value.points[index]);
    pointEdit.value = index;
  }

  pointTab.value = 0;
  resetPointTest();
};

const savePoint = () => {
  if (pointEdit.value === null) return;

  if (pointTab.value === 1) syncFrameToForm();
  if (!validatePointForm()) {
    pointTab.value = 0;
    return;
  }

  const pointData: IModbusPoint = { ...pointForm };

  const pointIndex = pointEdit.value < 0 ? draft.value.pointCount : pointEdit.value;
  const oldPoint = pointEdit.value >= 0 ? draft.value.points[pointEdit.value] : null;

  if (pointEdit.value < 0) {
    if (draft.value.pointCount >= 64) return;
    draft.value.points[draft.value.pointCount] = pointData;
    draft.value.pointCount += 1;
  } else {
    draft.value.points[pointEdit.value] = pointData;
  }

  // A master or local point is the source of its variable; link it automatically.
  if (pointData.var !== NONE && pointData.device !== DEVICE_SLAVE && draft.value.vars[pointData.var]) {
    draft.value.vars[pointData.var].point = pointIndex;
  }
  if (oldPoint && oldPoint.var !== NONE && oldPoint.var !== pointData.var && draft.value.vars[oldPoint.var]?.point === pointIndex) {
    draft.value.vars[oldPoint.var].point = NONE;
  }

  pointEdit.value = null;
  markDirty();
};

const askRemovePoint = (index: number) => {
  deleteTarget.value = { kind: 'point', index, name: pointName(index) };
};

const removeVarAt = (index: number) => {
  draft.value.vars.splice(index, 1);
  draft.value.vars.push(makeVar());
  draft.value.varCount -= 1;

  for (let i = 0; i < draft.value.pointCount; i++) {
    const current = draft.value.points[i].var;
    if (current === NONE) continue;
    if (current === index) draft.value.points[i].var = NONE;
    else if (current > index) draft.value.points[i].var -= 1;
  }
};

const performRemovePoint = (index: number) => {
  const removed = draft.value.points[index];
  draft.value.points.splice(index, 1);
  draft.value.points.push(makePoint());
  draft.value.pointCount -= 1;

  for (let i = 0; i < draft.value.varCount; i++) {
    const current = draft.value.vars[i].point;
    if (current === NONE) continue;
    if (current === index) draft.value.vars[i].point = NONE;
    else if (current > index) draft.value.vars[i].point -= 1;
  }

  if (removed?.device === DEVICE_SLAVE && removed.var !== NONE) {
    const used = draft.value.points.some((point, i) => i < draft.value.pointCount && point.var === removed.var);
    if (!used && removed.var < draft.value.varCount) removeVarAt(removed.var);
  }

  pointEdit.value = null;
  markDirty();
};

const openVar = (index?: number) => {
  clearErrors(varErrors);
  if (index === undefined) {
    if (draft.value.varCount >= 32) return;
    Object.assign(varForm, emptyVar(draft.value.varCount));
    varForm.point = draft.value.pointCount === 0 ? NONE : Math.min(varForm.point, draft.value.pointCount - 1);
    varEdit.value = -1;
  } else {
    Object.assign(varForm, draft.value.vars[index]);
    varEdit.value = index;
  }
};

const saveVar = () => {
  if (varEdit.value === null) return;
  if (!validateVarForm()) return;

  if (varEdit.value < 0) {
    if (draft.value.varCount >= 32) return;
    draft.value.vars[draft.value.varCount] = { ...varForm };
    draft.value.varCount += 1;
  } else {
    draft.value.vars[varEdit.value] = { ...varForm };
  }

  varEdit.value = null;
  markDirty();
};

const askRemoveVar = (index: number) => {
  deleteTarget.value = { kind: 'var', index, name: varName(index) };
};

const performRemoveVar = (index: number) => {
  removeVarAt(index);
  varEdit.value = null;
  markDirty();
};

const closePoint = () => {
  pointEdit.value = null;
};

const confirmDelete = () => {
  const target = deleteTarget.value;
  if (!target) return;

  if (target.kind === 'device') performRemoveDevice(target.index);
  else if (target.kind === 'point') performRemovePoint(target.index);
  else performRemoveVar(target.index);

  deleteTarget.value = null;
};

const writePoint = (index: number) => {
  const point = draft.value.points[index];
  if (!point) return;

  const rawValue = writeDrafts[index] ?? '';
  const text = point.type === TYPE_STRING ? rawValue : '';
  const numeric = point.type === TYPE_STRING ? 0 : toNumber(rawValue, 0);

  onSend(KEY.MODBUS, {
    command: CMD.WRITE,
    point: index,
    value: numeric,
    text: text,
  });
};

const sendProfile = (command: TCommand, action: TProfileAction) => {
  if (!profileValid.value || profileLoading.value || profileSending.value) return;

  clearProfileAck();
  profileError.value = '';
  profileMessage.value = '';
  profileSentSnapshot = JSON.stringify(normalizeProfile(draft.value));
  profileSending.value = action;
  profileAckTimer = setTimeout(() => {
    profileAckTimer = null;
    if (!profileSending.value) return;
    profileSending.value = null;
    profileError.value = $t('modbus.profileNoResponse');
  }, 3000);

  onSend(KEY.MODBUSPROFILE, { ...draft.value, command });
};

const onApplyProfile = () => sendProfile(CMD.SET, 'apply');
const onSaveProfile = () => sendProfile(CMD.SAVE, 'save');

const onExport = () => {
  if (profileLoading.value || profileSending.value) return;
  const buffer = struct.set('MODBUSPROFILE', { ...draft.value, command: 0, key: KEY.MODBUSPROFILE });
  const blob = new Blob([buffer], { type: 'application/octet-stream' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = 'modbus.bin';
  link.click();
  URL.revokeObjectURL(url);
};

const onImport = async (event: Event) => {
  if (profileLoading.value || profileSending.value) return;
  profileError.value = '';
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  if (!file) return;

  try {
    const buffer = await file.arrayBuffer();
    const decoded = struct.get(buffer);
    if (!decoded || decoded.key !== KEY.MODBUSPROFILE) {
      profileError.value = $t('modbus.importError');
      return;
    }
    draft.value = normalizeProfile(decoded.object as unknown as IModbusProfile);
    markDirty();
  } catch {
    profileError.value = $t('modbus.importError');
  } finally {
    input.value = '';
  }
};

const crc16 = (bytes: number[]) => {
  let crc = 0xffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit++) {
      crc = crc & 1 ? (crc >> 1) ^ 0xa001 : crc >> 1;
    }
  }
  return crc;
};

const parseHex = (value: string) => {
  const clean = value.replace(/0x/gi, '').replace(/[^0-9a-f]/gi, '');
  if (!clean || clean.length % 2 !== 0) return [];
  const bytes: number[] = [];
  for (let i = 0; i < clean.length; i += 2) bytes.push(parseInt(clean.slice(i, i + 2), 16));
  return bytes;
};

const toHex = (bytes: number[]) => bytes.map((byte) => byte.toString(16).toUpperCase().padStart(2, '0')).join(' ');

const withCrc = (bytes: number[]) => {
  const crc = crc16(bytes);
  return [...bytes, crc & 0xff, (crc >> 8) & 0xff];
};

function registersForType(type: number, stringRegisters: number) {
  if (type === TYPE_STRING) return Math.min(16, Math.max(1, stringRegisters || 1));
  if (type === TYPE_U32 || type === TYPE_S32 || type === TYPE_FLOAT32) return 2;
  if (type === TYPE_U64 || type === TYPE_S64 || type === TYPE_FLOAT64) return 4;
  return 1;
}

const pointTestCount = computed(() => {
  if (pointForm.fc <= 2) return 1;
  const minimum = registersForType(pointForm.type, pointForm.registers);
  return Math.min(16, Math.max(minimum, pointForm.registers || 1));
});

const pointTestPayload = () => {
  const slave = draft.value.devices[pointForm.device]?.address ?? 1;
  return [slave, pointForm.fc, (pointForm.address >> 8) & 0xff, pointForm.address & 0xff, 0, pointTestCount.value];
};

const testFrameBytes = computed(() => parseHex(testFrame.value));

const testSummary = computed(() => {
  const bytes = testFrameBytes.value;
  const slave = bytes.length >= 6 ? bytes[0] : pointTestPayload()[0];
  const fc = bytes.length >= 6 ? bytes[1] : pointForm.fc;
  const address = bytes.length >= 6 ? (bytes[2] << 8) | bytes[3] : pointForm.address;
  const count = bytes.length >= 6 ? (bytes[4] << 8) | bytes[5] : pointTestCount.value;
  return `${slave} · FC ${String(fc).padStart(2, '0')} · ${address} · ${count}`;
});

const testValid = computed(() => testFrameBytes.value.length >= 6 && draft.value.deviceCount > 0);

const rebuildTestFrame = () => {
  const payload = pointTestPayload();
  let bytes: number[];

  if (testAutoCrc.value) {
    bytes = withCrc(payload);
  } else {
    const current = parseHex(testFrame.value);
    const crc = current.length >= 2 ? current.slice(-2) : [0, 0];
    bytes = [...payload, ...crc];
  }

  testFrame.value = toHex(bytes);
  testResult.value = null;
  testError.value = '';
};

const setTestAutoCrc = (value: boolean) => {
  const current = parseHex(testFrame.value);
  testAutoCrc.value = value;

  if (value) {
    const payload = current.length >= 2 ? current.slice(0, -2) : pointTestPayload();
    testFrame.value = toHex(withCrc(payload));
  } else {
    testFrame.value = toHex(current.length >= 2 ? current : withCrc(pointTestPayload()));
  }

  testResult.value = null;
  testError.value = '';
};

const onTestFrameInput = (value: string) => {
  testFrame.value = value;
  testResult.value = null;
  testError.value = '';
};

const syncFrameToForm = () => {
  const bytes = parseHex(testFrame.value);
  if (bytes.length < 6) return;

  const slave = bytes[0];
  const fc = bytes[1];
  const address = (bytes[2] << 8) | bytes[3];
  const count = (bytes[4] << 8) | bytes[5];

  testSyncing.value = true;
  const device = draft.value.devices.findIndex((item, index) => index < draft.value.deviceCount && item.address === slave);
  if (device >= 0) pointForm.device = device;
  if (fc >= 1 && fc <= 4) pointForm.fc = fc;
  pointForm.address = address;
  if (fc === 3 || fc === 4) pointForm.registers = Math.min(16, Math.max(1, count));

  nextTick(() => {
    testSyncing.value = false;
  });
};

const onPointTab = (index: number) => {
  if (index === pointTab.value) return;

  if (index === 0) syncFrameToForm();
  else rebuildTestFrame();

  pointTab.value = index;
};

// Point fields and the frame are two views of the same request. Manual frame
// edits write back to the form, while form edits rebuild the frame.
watch(
  () => [pointForm.device, pointForm.fc, pointForm.address, pointForm.type, pointForm.registers, pointForm.flags],
  () => {
    if (testSyncing.value || pointEdit.value === null) return;
    rebuildTestFrame();
  }
);

const resetPointTest = () => {
  if (testTimer) clearTimeout(testTimer);
  testTimer = null;
  testAutoCrc.value = true;
  testPending.value = false;
  pointTab.value = 0;
  testResult.value = null;
  testError.value = '';
  rebuildTestFrame();
};

const testStatusName = (status: number) => {
  const labels: Record<number, string> = {
    0: $t('modbus.rawIdle'),
    1: $t('modbus.rawWait'),
    2: $t('modbus.rawOk'),
    3: $t('modbus.rawTimeout'),
    4: $t('modbus.rawCrcError'),
    5: $t('modbus.rawException'),
    6: $t('modbus.rawUnavailable'),
    7: $t('modbus.rawInvalid'),
    8: $t('modbus.rawBusy'),
  };
  return labels[status] ?? $t('modbus.rawIdle');
};

const testStatusClass = (status: number) => {
  if (status === 2) return 'text-green-600 dark:text-green-400';
  if (status === 1) return 'text-amber-600 dark:text-amber-400';
  return 'text-red-600 dark:text-red-400';
};

const testResponse = computed(() => {
  const raw = testResult.value;
  if (!raw) return '';
  const bytes = (raw.response ?? []).slice(0, raw.responseLen ?? 0);
  return bytes.length ? toHex(bytes) : '';
});

const testException = computed(() => {
  const raw = testResult.value;
  if (!raw || raw.status !== 5) return null;
  const bytes = raw.response ?? [];
  return raw.responseLen > 2 ? bytes[2] : null;
});

const decodeTestValue = () => {
  const raw = testResult.value;
  if (!raw || raw.status !== 2) return '—';

  const request = parseHex(testFrame.value);
  const fc = request.length >= 2 ? request[1] : pointForm.fc;
  const count = request.length >= 6 ? (request[4] << 8) | request[5] : pointTestCount.value;
  const bytes = (raw.response ?? []).slice(0, raw.responseLen ?? 0);

  if (fc === 1 || fc === 2) {
    if (bytes.length < 4) return '—';
    return bytes[3] & 0x01 ? 'ON' : 'OFF';
  }

  const data = bytes.slice(3, 3 + Math.max(1, count) * 2);
  if (data.length < 2) return '—';

  if (pointForm.flags & 2) {
    for (let i = 0; i + 1 < data.length; i += 2) {
      const swap = data[i];
      data[i] = data[i + 1];
      data[i + 1] = swap;
    }
  }

  if (pointForm.flags & 1) {
    const registers: number[][] = [];
    for (let i = 0; i + 1 < data.length; i += 2) registers.push([data[i], data[i + 1]]);
    registers.reverse();
    data.splice(0, data.length, ...registers.flat());
  }

  const view = new DataView(new Uint8Array(data).buffer);
  let rawValue = 0;

  switch (pointForm.type) {
    case TYPE_S16:
      rawValue = view.getInt16(0);
      break;
    case TYPE_U32:
      rawValue = data.length >= 4 ? view.getUint32(0) : 0;
      break;
    case TYPE_S32:
      rawValue = data.length >= 4 ? view.getInt32(0) : 0;
      break;
    case TYPE_FLOAT32:
      rawValue = data.length >= 4 ? view.getFloat32(0) : 0;
      break;
    case TYPE_U64:
      rawValue = data.length >= 8 ? Number(view.getBigUint64(0)) : 0;
      break;
    case TYPE_S64:
      rawValue = data.length >= 8 ? Number(view.getBigInt64(0)) : 0;
      break;
    case TYPE_FLOAT64:
      rawValue = data.length >= 8 ? view.getFloat64(0) : 0;
      break;
    case TYPE_STRING:
      return String.fromCharCode(...data).replace(/\0.*$/, '') || '—';
    case TYPE_BCD: {
      const value = view.getUint16(0);
      rawValue = ((value >> 12) & 0xf) * 1000 + ((value >> 8) & 0xf) * 100 + ((value >> 4) & 0xf) * 10 + (value & 0xf);
      break;
    }
    default:
      rawValue = view.getUint16(0);
      break;
  }

  const value = rawValue * (pointForm.scale || 1) + pointForm.offset;
  return `${value.toFixed(pointForm.decimals || 0)}${pointForm.unit ? ` ${pointForm.unit}` : ''}`;
};

const testDecoded = computed(decodeTestValue);

const onPointTest = () => {
  if (testPending.value) return;

  if (testAutoCrc.value) {
    const current = parseHex(testFrame.value);
    const payload = current.length >= 2 ? current.slice(0, -2) : pointTestPayload();
    testFrame.value = toHex(withCrc(payload));
  }

  const bytes = parseHex(testFrame.value);
  if (bytes.length < 4) {
    testError.value = $t('modbus.rawInvalid');
    return;
  }

  if (testTimer) clearTimeout(testTimer);
  testResult.value = null;
  testError.value = '';
  testPending.value = true;

  testTimer = setTimeout(() => {
    if (!testPending.value) return;
    testPending.value = false;
    testError.value = $t('modbus.rawTimeout');
  }, 2500);

  onSend(KEY.MODBUSRAW, {
    command: CMD.SET,
    slave: bytes[0],
    function: bytes[1],
    address: (bytes[2] << 8) | bytes[3],
    length: bytes.length >= 6 ? (bytes[4] << 8) | bytes[5] : 1,
    autoCrc: 0,
    request: Array.from({ length: 64 }, (_, i) => bytes[i] ?? 0),
    requestLen: bytes.length,
    response: Array.from({ length: 64 }, () => 0),
    responseLen: 0,
    status: 0,
    reserved: 0,
    elapsed: 0,
  });
};

const onRole = ({ value }: IListItem<number>) => {
  form.role = value;
  dirty.value = true;
};

const onBaud = ({ value }: IListItem<number>) => {
  form.baud = value;
  dirty.value = true;
};

const onParity = ({ value }: IListItem<number>) => {
  form.parity = value;
  dirty.value = true;
};

const onStopBits = ({ value }: IListItem<number>) => {
  form.stopBits = value;
  dirty.value = true;
};

const onAddress = (value: string) => {
  form.address = clamp(value, 1, 247);
  dirty.value = true;
};

const onApply = () => {
  onSend(KEY.MODBUS, {
    ...main.value.modbus,
    role: form.role,
    address: form.address,
    baud: form.baud,
    dataBits: 8,
    parity: form.parity,
    stopBits: form.stopBits,
    command: CMD.SET,
  });
};

const onSave = () => {
  onSend(KEY.MODBUS, {
    ...main.value.modbus,
    role: form.role,
    address: form.address,
    baud: form.baud,
    dataBits: 8,
    parity: form.parity,
    stopBits: form.stopBits,
    command: CMD.SAVE,
  });
};
</script>
