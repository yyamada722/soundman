<template>
  <div class="home">
    <div class="welcome-card">
      <h2>Welcome to Soundman Web!</h2>
      <p class="subtitle">Audio Library Management System</p>

      <div class="status">
        <div class="status-item success">
          <span class="icon">✓</span>
          <span>Project successfully initialized</span>
        </div>
      </div>

      <div class="info-section">
        <h3>What's Next?</h3>
        <ol class="next-steps">
          <li>Install dependencies: <code>npm install</code></li>
          <li>Implement library store in <code>src/stores/library.ts</code></li>
          <li>Create UI components in <code>src/components/</code></li>
          <li>Add type definitions in <code>src/types/</code></li>
          <li>Build services in <code>src/services/</code></li>
        </ol>
      </div>

      <div class="info-section">
        <h3>Features to Implement (Phase 1)</h3>
        <ul class="features-list">
          <li>File list view (grid/list)</li>
          <li>Metadata display and editing</li>
          <li>Search and filtering</li>
          <li>Audio preview player</li>
          <li>Drag & drop file import</li>
        </ul>
      </div>

      <div class="docs-link">
        <p>See <a href="../docs/ROADMAP.md">docs/ROADMAP.md</a> for the full development plan</p>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// Component logic will be added here
</script>

<style scoped>
.home {
  padding: 1rem;
}

.welcome-card {
  background: white;
  border-radius: 8px;
  padding: 2rem;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

h2 {
  margin: 0 0 0.5rem;
  color: #333;
  font-size: 2rem;
}

.subtitle {
  margin: 0 0 2rem;
  color: #666;
  font-size: 1.125rem;
}

.status {
  margin-bottom: 2rem;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 1rem;
  border-radius: 6px;
  font-weight: 500;
}

.status-item.success {
  background: #d4edda;
  color: #155724;
}

.icon {
  font-size: 1.5rem;
}

.info-section {
  margin: 2rem 0;
}

h3 {
  color: #444;
  font-size: 1.25rem;
  margin-bottom: 1rem;
}

.next-steps {
  list-style: decimal;
  padding-left: 1.5rem;
  color: #555;
}

.next-steps li {
  margin-bottom: 0.75rem;
  line-height: 1.6;
}

.features-list {
  list-style: disc;
  padding-left: 1.5rem;
  color: #555;
}

.features-list li {
  margin-bottom: 0.5rem;
}

code {
  background: #f4f4f4;
  padding: 0.2rem 0.4rem;
  border-radius: 3px;
  font-family: 'Courier New', monospace;
  font-size: 0.9em;
  color: #e83e8c;
}

.docs-link {
  margin-top: 2rem;
  padding-top: 2rem;
  border-top: 1px solid #e0e0e0;
  text-align: center;
  color: #666;
}

.docs-link a {
  color: #667eea;
  text-decoration: none;
  font-weight: 500;
}

.docs-link a:hover {
  text-decoration: underline;
}
</style>
