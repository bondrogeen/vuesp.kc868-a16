<template>
  <!-- Глобальный декоративный слой: виден на всех страницах. -->
  <div aria-hidden="true" class="pointer-events-none fixed inset-0 z-0 overflow-hidden">
    <span class="page-backdrop-dot absolute left-[8%] top-[22%] size-1 rounded-full bg-cyan-main/50"></span>
    <span class="page-backdrop-dot absolute left-[62%] top-[12%] size-2 rounded-full bg-violet-main/40" style="animation-delay: 1.4s"></span>
    <span class="page-backdrop-dot absolute left-[78%] top-[68%] size-1 rounded-full bg-blue-main/50" style="animation-delay: 2.8s"></span>
    <span class="page-backdrop-dot absolute left-[24%] top-[82%] size-1 rounded-full bg-cyan-main/40" style="animation-delay: 4.2s"></span>

    <span class="page-backdrop-orbit absolute -right-24 -top-24 size-72 rounded-full border border-dashed border-cyan-main/10"></span>
    <span class="page-backdrop-orbit absolute -bottom-32 -left-24 size-96 rounded-full border border-dashed border-violet-main/10" style="animation-delay: -13s"></span>
  </div>
</template>

<style scoped>
/* Декоративная анимация: @keyframes нельзя выразить Tailwind-утилитами. */
.page-backdrop-dot {
  animation: page-backdrop-float 10s ease-in-out infinite;
}

.page-backdrop-orbit {
  animation: page-backdrop-spin 32s linear infinite;
}

@keyframes page-backdrop-float {
  0%,
  100% {
    transform: translate3d(0, 0, 0);
    opacity: 0.35;
  }

  50% {
    transform: translate3d(0, -14px, 0);
    opacity: 0.9;
  }
}

@keyframes page-backdrop-spin {
  to {
    transform: rotate(360deg);
  }
}

@media (prefers-reduced-motion: reduce) {
  .page-backdrop-dot,
  .page-backdrop-orbit {
    animation: none;
  }
}
</style>
