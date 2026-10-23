type ProgressCallback = (progress: number) => void;

export function createSteps(
  steps: number[] | number,
  onProgress: ProgressCallback,
): ProgressCallback[] {
  const stepWeights = typeof steps === "number" ? Array(steps).fill(1) : steps;
  const progressParts = Array(stepWeights.length).fill(0);
  const totalSize = stepWeights.reduce((total, weight) => total + weight, 0);

  function updateProgress() {
    const weightedAverage = stepWeights.reduce(
      (acc, weight, idx) => acc + progressParts[idx] * weight,
      0,
    );
    onProgress(weightedAverage / totalSize);
  }

  return stepWeights.map((_weight, idx) => (progress: number) => {
    if (progressParts[idx] !== progress) {
      progressParts[idx] = progress;
      updateProgress();
    }
  });
}

export function withProgress<T>(
  steps: T[],
  onProgress: ProgressCallback,
  getStepWeight?: (step: T) => number,
): [T, ProgressCallback][] {
  const callbacks = createSteps(
    steps.map(
      getStepWeight ||
        ((step: any) =>
          typeof step === "number"
            ? step
            : typeof step !== "string"
              ? step.size || step.length || 1
              : 1),
    ),
    onProgress,
  );
  return steps.map((step, idx) => [step, callbacks[idx]]);
}
