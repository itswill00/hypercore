package com.hypermoon;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class HyperMoonOverlay {
    private static Context context;
    private static WindowManager windowManager;
    private static Context sysContext;
    private static Display defaultDisplay;
    private static CanvasHudView hudView;
    private static WindowManager.LayoutParams params;
    private static Handler handler;

    // Direct Hardware SurfaceControl Engine State (Direct SurfaceFlinger - Bypasses WMS & AMS)
    private static boolean isHardwareSurface = false;
    private static Object surfaceControl = null;
    private static Object surfaceControlTx = null;
    private static Object hardwareSurface = null;
    private static int currentScW = 0;
    private static int currentScH = 0;
    private static Method scLockCanvas = null;
    private static Method scUnlockCanvasAndPost = null;
    private static Method txSetPos = null;
    private static Method txSetVis = null;
    private static Method txSetLayer = null;
    private static Method txSetLayerStack = null;
    private static Method txApply = null;
    private static Method txRemove = null;

    // Config & State Paths
    private static String stateDir   = "/data/adb/hypercore/hud";
    private static String statePath  = stateDir + "/stats.json";
    private static String configPath = stateDir + "/config.json";
    private static String posPath    = stateDir + "/position.json";

    // Layout State Variables
    private static volatile boolean isDragging = false;
    private static int posX = 697;
    private static int posY = 411;
    private static float scale = 0.65f;
    private static float opacity = 0.70f;
    private static int fontSizeSp = 14;
    private static int cornerRadiusDp = 14;
    private static int bgWidthDp = 150;
    private static int bgHeightDp = 160;
    private static int refreshInterval = 1500;
    private static String theme = "cyber_neon";
    private static String customHexColor = "#6366F1";
    private static boolean isHorizontal = false;
    private static String align = "left"; // left, center, right
    private static boolean isVisible = true;
    private static boolean autoGaming = false;

    // Config & Position timestamp check — instant sync on file modification
    private static long lastConfigModified = 0L;
    private static long lastPosModified = 0L;

    // Metric Toggles
    private static boolean showFps = true;
    private static boolean showCpu = true;
    private static boolean showCpuFreq = true;
    private static boolean showGov = false;
    private static boolean showGpu = true;
    private static boolean showGpuFreq = true;
    private static boolean showGpuGov = false;
    private static boolean showRam = true;
    private static boolean showZram = false;
    private static boolean showBattery = true;
    private static boolean showNet = true;

    // Render Stats State
    private static String fpsText = "60";
    private static String ftText = "16.6";
    private static String hzText = "60Hz";
    private static String cpuText = "CPU 45°C (35%)";
    private static String govText = "schedutil";
    private static String gpuText = "GPU 72%";
    private static String gpuGovText = "ged_dvfs";
    private static String ramText = "RAM 4.7/7.5G";
    private static String zramText = "ZRAM 1.8/6.0G";
    private static String pwrText = "PWR 3.45W";
    private static String netText = "DL 0 KB/s";

    // Shared Graphics Objects (Zero Allocation) - Initialized safely in initGraphics() after Typeface setup
    private static Paint paintBg;
    private static Paint paintFpsNum;
    private static Paint paintFpsLbl;
    private static Paint paintSubText;
    private static RectF rectBg;
    private static Paint measurePaintNum;
    private static Paint measurePaintSub;

    private static void initGraphics() {
        if (paintBg != null) return;
        paintBg = new Paint(Paint.ANTI_ALIAS_FLAG);
        paintBg.setStyle(Paint.Style.FILL);

        paintFpsNum = new Paint(Paint.ANTI_ALIAS_FLAG);
        try {
            paintFpsNum.setTypeface(Typeface.create("sans-serif-medium", Typeface.BOLD));
        } catch (Throwable t) {
            try { paintFpsNum.setTypeface(Typeface.DEFAULT_BOLD); } catch (Throwable ignored) {}
        }

        paintFpsLbl = new Paint(Paint.ANTI_ALIAS_FLAG);
        paintFpsLbl.setColor(Color.parseColor("#94A3B8"));
        try {
            paintFpsLbl.setTypeface(Typeface.create("sans-serif", Typeface.NORMAL));
        } catch (Throwable t) {
            try { paintFpsLbl.setTypeface(Typeface.DEFAULT); } catch (Throwable ignored) {}
        }

        paintSubText = new Paint(Paint.ANTI_ALIAS_FLAG);
        paintSubText.setColor(Color.parseColor("#E2E8F0"));
        try {
            paintSubText.setTypeface(Typeface.create("sans-serif", Typeface.NORMAL));
        } catch (Throwable t) {
            try { paintSubText.setTypeface(Typeface.DEFAULT); } catch (Throwable ignored) {}
        }

        rectBg = new RectF();

        measurePaintNum = new Paint();
        try {
            measurePaintNum.setTypeface(Typeface.create("sans-serif-medium", Typeface.BOLD));
        } catch (Throwable t) {
            try { measurePaintNum.setTypeface(Typeface.DEFAULT_BOLD); } catch (Throwable ignored) {}
        }

        measurePaintSub = new Paint();
        try {
            measurePaintSub.setTypeface(Typeface.create("sans-serif", Typeface.NORMAL));
        } catch (Throwable t) {
            try { measurePaintSub.setTypeface(Typeface.DEFAULT); } catch (Throwable ignored) {}
        }
    }

    static class CanvasHudView extends View {
        public CanvasHudView(Context context) {
            super(context);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int[] dims = calcHudDimensions();
            int measuredW = MeasureSpec.getMode(widthMeasureSpec) != MeasureSpec.UNSPECIFIED
                    ? MeasureSpec.getSize(widthMeasureSpec) : dims[0];
            int measuredH = MeasureSpec.getMode(heightMeasureSpec) != MeasureSpec.UNSPECIFIED
                    ? MeasureSpec.getSize(heightMeasureSpec) : dims[1];
            setMeasuredDimension(measuredW, measuredH);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            int[] dims = calcHudDimensions();
            float cardW = getWidth() > 0 ? getWidth() : dims[0];
            float cardH = getHeight() > 0 ? getHeight() : dims[1];
            drawHud(canvas, cardW, cardH);
        }
    }

    private static void drawHud(Canvas canvas, float cardW, float cardH) {
        try {
            // 1. Draw Glassmorphism Background Card (Solid & Consistent)
            int bgColor;
            if ("amoled".equalsIgnoreCase(theme)) {
                bgColor = Color.argb((int)(opacity * 255), 0, 0, 0);
            } else if ("matrix".equalsIgnoreCase(theme)) {
                bgColor = Color.argb((int)(opacity * 255), 6, 22, 14);
            } else if ("crimson".equalsIgnoreCase(theme)) {
                bgColor = Color.argb((int)(opacity * 255), 24, 8, 12);
            } else if ("custom".equalsIgnoreCase(theme) && customHexColor != null && customHexColor.startsWith("#")) {
                try {
                    int c = Color.parseColor(customHexColor);
                    bgColor = Color.argb((int)(opacity * 255), Color.red(c), Color.green(c), Color.blue(c));
                } catch (Exception e) {
                    bgColor = Color.argb((int)(opacity * 255), 11, 13, 20);
                }
            } else {
                bgColor = Color.argb((int)(opacity * 255), 11, 13, 20);
            }

            paintBg.setColor(bgColor);
            float r = dpToPx(cornerRadiusDp) * scale;
            rectBg.set(0, 0, cardW, cardH);
            canvas.drawRoundRect(rectBg, r, r, paintBg);

            // 2. Consistent Sleek FPS Text Color (No border, No jarring color shift during games)
            String drawFps = (fpsText != null) ? fpsText : "0";
            if ("matrix".equalsIgnoreCase(theme)) {
                paintFpsNum.setColor(Color.parseColor("#22C55E"));
            } else if ("crimson".equalsIgnoreCase(theme)) {
                paintFpsNum.setColor(Color.parseColor("#F43F5E"));
            } else {
                paintFpsNum.setColor(Color.parseColor("#38BDF8"));
            }

            // 3. Dynamic Alignment & Padding Calculation
            float padX;
            Paint.Align paintAlign;

            if ("center".equalsIgnoreCase(align)) {
                padX = cardW / 2.0f;
                paintAlign = Paint.Align.CENTER;
            } else if ("right".equalsIgnoreCase(align)) {
                padX = cardW - (dpToPx(14) * scale);
                paintAlign = Paint.Align.RIGHT;
            } else {
                padX = dpToPx(14) * scale;
                paintAlign = Paint.Align.LEFT;
            }

            paintFpsNum.setTextAlign(paintAlign);
            paintFpsLbl.setTextAlign(paintAlign);
            paintSubText.setTextAlign(paintAlign);

            String drawFt = (ftText != null) ? ftText : "0.0";
            String drawHz = (hzText != null) ? hzText : "60Hz";

            float fpsNumSize = spToPx(fontSizeSp + 7) * scale;
            float subTextSize = spToPx(Math.max(9, fontSizeSp - 1)) * scale;

            paintFpsNum.setTextSize(fpsNumSize);
            paintFpsLbl.setTextSize(subTextSize);
            paintSubText.setTextSize(subTextSize);

            if (isHorizontal) {
                float totalTextH = fpsNumSize + (dpToPx(4) * scale) + subTextSize;
                float startY = (cardH - totalTextH) / 2.0f;
                float yRow1 = startY + (fpsNumSize * 0.82f);
                float yRow2 = yRow1 + (dpToPx(4) * scale) + (subTextSize * 0.82f);

                if ("center".equalsIgnoreCase(align) || "right".equalsIgnoreCase(align)) {
                    String fullLine = drawFps + " FPS (" + drawFt + " ms) [" + drawHz + "]";
                    canvas.drawText(fullLine, padX, yRow1, paintFpsNum);
                } else {
                    canvas.drawText(drawFps, padX, yRow1, paintFpsNum);
                    float fpsWidth = paintFpsNum.measureText(drawFps);
                    canvas.drawText(" FPS  (" + drawFt + " ms) [" + drawHz + "]", padX + fpsWidth, yRow1, paintFpsLbl);
                }

                // Row 2 Hardware Metrics Chips
                StringBuilder sbChips = new StringBuilder();
                if (showCpu && cpuText != null) sbChips.append(cpuText);
                if (showGov && govText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(govText);
                if (showGpu && gpuText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(gpuText);
                if (showGpuGov && gpuGovText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append("GPUGov: ").append(gpuGovText);
                if (showRam && ramText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(ramText);
                if (showZram && zramText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(zramText);
                if (showBattery && pwrText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(pwrText);
                if (showNet && netText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(netText);

                canvas.drawText(sbChips.toString(), padX, yRow2, paintSubText);

            } else {
                // Vertical Stack Layout with Dynamic Line Spacing
                float lineGap = subTextSize * 1.45f;
                float yCurr = (dpToPx(16) * scale) + fpsNumSize * 0.75f;

                canvas.drawText(drawFps + " FPS", padX, yCurr, paintFpsNum);

                yCurr += lineGap;
                canvas.drawText(drawFt + " ms  [" + drawHz + "]", padX, yCurr, paintFpsLbl);

                if (showCpu && cpuText != null) {
                    yCurr += lineGap;
                    canvas.drawText(cpuText, padX, yCurr, paintSubText);
                }
                if (showGov && govText != null) {
                    yCurr += lineGap;
                    canvas.drawText("CPUGov: " + govText, padX, yCurr, paintSubText);
                }
                if (showGpu && gpuText != null) {
                    yCurr += lineGap;
                    canvas.drawText(gpuText, padX, yCurr, paintSubText);
                }
                if (showGpuGov && gpuGovText != null) {
                    yCurr += lineGap;
                    canvas.drawText("GPUGov: " + gpuGovText, padX, yCurr, paintSubText);
                }
                if (showRam && ramText != null) {
                    yCurr += lineGap;
                    canvas.drawText(ramText, padX, yCurr, paintSubText);
                }
                if (showZram && zramText != null) {
                    yCurr += lineGap;
                    canvas.drawText(zramText, padX, yCurr, paintSubText);
                }
                if (showBattery && pwrText != null) {
                    yCurr += lineGap;
                    canvas.drawText(pwrText, padX, yCurr, paintSubText);
                }
                if (showNet && netText != null) {
                    yCurr += lineGap;
                    canvas.drawText(netText, padX, yCurr, paintSubText);
                }
            }
        } catch (Throwable t) {
            // Fail-safe protection against canvas rendering crashes
        }
    }

    public static void main(String[] args) {
        System.out.println("[HyperMoon] Starting overlay service...");

        if (args != null && args.length > 0 && args[0] != null && !args[0].trim().isEmpty()) {
            stateDir = args[0].trim();
        } else {
            String envState = System.getenv("HYPERMOON_STATE_DIR");
            if (envState == null || envState.trim().isEmpty()) {
                envState = System.getenv("FPSMOON_STATE_DIR");
            }
            if (envState != null && !envState.trim().isEmpty()) {
                stateDir = envState.trim();
            }
        }
        statePath  = stateDir + "/stats.json";
        configPath = stateDir + "/config.json";
        posPath    = stateDir + "/position.json";

        try {
            Looper.prepareMainLooper();

            // 1. Initialize System Font Map and set Default Typeface natively (prevents AOSP / minikin SIGABRT)
            try {
                Method fontMapMethod = Typeface.class.getDeclaredMethod("loadPreinstalledSystemFontMap");
                fontMapMethod.setAccessible(true);
                fontMapMethod.invoke(null);
            } catch (Throwable ignored) {}

            try {
                Method getMapMethod = Typeface.class.getDeclaredMethod("getSystemFontMap");
                getMapMethod.setAccessible(true);
                Map<?, ?> map = (Map<?, ?>) getMapMethod.invoke(null);
                if (map != null) {
                    Object sansSerif = map.get("sans-serif");
                    if (sansSerif instanceof Typeface) {
                        Method setDef = Typeface.class.getDeclaredMethod("setDefault", Typeface.class);
                        setDef.setAccessible(true);
                        setDef.invoke(null, (Typeface) sansSerif);
                    }
                }
            } catch (Throwable ignored) {}

            // 2. Safe Graphics Initialization (Executed after System Font Map is ready)
            initGraphics();

            // 3. Initialize libbinder Worker Thread Pool for system_server IPC callbacks (prevents AOSP WM leash timeouts)
            try {
                Class<?> binderInternal = Class.forName("com.android.internal.os.BinderInternal");
                try {
                    Method disableBg = binderInternal.getMethod("disableBackgroundScheduling", boolean.class);
                    disableBg.invoke(null, true);
                } catch (Throwable ignored) {}

                try {
                    final Method joinThreadPool = binderInternal.getMethod("joinThreadPool");
                    Thread binderThread = new Thread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                joinThreadPool.invoke(null);
                            } catch (Throwable ignored) {}
                        }
                    }, "FPSMoon-BinderPool");
                    binderThread.setDaemon(true);
                    binderThread.start();
                } catch (Throwable ignored) {}
            } catch (Throwable ignored) {}

            // 3. Acquire System Context via ActivityThread
            try {
                Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
                Method systemMainMethod = activityThreadClass.getMethod("systemMain");
                Object activityThread = systemMainMethod.invoke(null);
                sysContext = (Context) activityThreadClass.getMethod("getSystemContext").invoke(activityThread);
            } catch (Throwable t) {
                System.err.println("[HyperMoon Error] ActivityThread.systemMain failed: " + t.getMessage());
            }

            if (sysContext == null) {
                System.err.println("[HyperMoon ERROR] Unable to acquire system context. Stopping overlay.");
                return;
            }

            // 4. Dynamically Grant AppOps SYSTEM_ALERT_WINDOW across UIDs (root=0, system=1000, shell=2000)
            try {
                Class<?> appOpsClass = Class.forName("android.app.AppOpsManager");
                Object appOps = sysContext.getSystemService(Context.APP_OPS_SERVICE);
                if (appOps != null) {
                    Method setUidModeMethod = null;
                    for (Method m : appOpsClass.getDeclaredMethods()) {
                        if ("setUidMode".equals(m.getName()) && m.getParameterTypes().length >= 3) {
                            setUidModeMethod = m;
                            break;
                        }
                    }
                    if (setUidModeMethod != null) {
                        setUidModeMethod.setAccessible(true);
                        int myUid = Process.myUid();
                        setUidModeMethod.invoke(appOps, 24 /* OP_SYSTEM_ALERT_WINDOW */, myUid, 0 /* MODE_ALLOWED */);
                        setUidModeMethod.invoke(appOps, 24, 1000, 0);
                        setUidModeMethod.invoke(appOps, 24, 2000, 0);
                    }
                }
            } catch (Throwable ignored) {}

            // 5. Context & Display Resolution
            context = sysContext; // Primary fallback
            try {
                DisplayManager dm = (DisplayManager) sysContext.getSystemService(Context.DISPLAY_SERVICE);
                if (dm != null) {
                    defaultDisplay = dm.getDisplay(Display.DEFAULT_DISPLAY);
                    if (defaultDisplay != null) {
                        Context displayCtx = sysContext.createDisplayContext(defaultDisplay);
                        if (displayCtx != null) {
                            context = displayCtx;
                        }
                    }
                }
            } catch (Throwable t) {
                System.err.println("[HyperMoon Warning] DisplayContext fallback: " + t.getMessage());
            }

            try {
                windowManager = (WindowManager) sysContext.getSystemService(Context.WINDOW_SERVICE);
            } catch (Throwable t) {
                System.err.println("[HyperMoon Warning] sysContext WINDOW_SERVICE: " + t.getMessage());
            }

            if (windowManager == null) {
                try {
                    windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
                } catch (Throwable ignored) {}
            }

            if (windowManager == null) {
                System.err.println("[HyperMoon Warning] Unable to acquire WindowManager service. Hardware SurfaceControl fallback will be used.");
            }

            handler = new Handler(Looper.getMainLooper());

            // Register Graceful WindowManager & SurfaceControl Cleanup Shutdown Hook
            Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {
                @Override
                public void run() {
                    if (!isHardwareSurface && windowManager != null && hudView != null) {
                        try {
                            windowManager.removeView(hudView);
                        } catch (Exception ignored) {}
                    } else if (isHardwareSurface) {
                        try {
                            if (surfaceControlTx != null && txRemove != null && surfaceControl != null) {
                                txRemove.invoke(surfaceControlTx, surfaceControl);
                                txApply.invoke(surfaceControlTx);
                            }
                        } catch (Exception ignored) {}
                    }
                }
            }));

            readConfig();
            readPosition();
            createHudWindow();
            startLoop();

            System.out.println("[HyperMoon] Overlay started successfully.");

            while (true) {
                try {
                    Looper.loop();
                } catch (Throwable t) {
                    System.err.println("[HyperMoon] Looper crash: " + t.getMessage());
                    try { Thread.sleep(100); } catch (InterruptedException ignored) {}
                }
            }
        } catch (Exception e) {
            System.err.println("[HyperMoon] Error in overlay:");
            e.printStackTrace();
        }
    }

    // Reusable Measurement Data & Result Array (Zero Heap GC Overhead)
    private static final int[] dimsResult = new int[2];
    private static final Pattern STATS_PATTERN = Pattern.compile("\"([^\"]+)\"\\s*:\\s*(?:\"([^\"]*)\"|(-?[\\d.]+|(?:true|false)))");

    /** Single source of truth for smart HUD dimensions (Width and Height in Px) */
    private static int[] calcHudDimensions() {
        if (measurePaintNum == null) {
            initGraphics();
        }

        if (context == null) {
            dimsResult[0] = 400;
            dimsResult[1] = 100;
            return dimsResult;
        }

        float fpsNumSize = spToPx(fontSizeSp + 7) * scale;
        float subTextSize = spToPx(Math.max(9, fontSizeSp - 1)) * scale;

        measurePaintNum.setTextSize(fpsNumSize);
        measurePaintSub.setTextSize(subTextSize);

        String drawFps = (fpsText != null) ? fpsText : "0";
        String drawFt = (ftText != null) ? ftText : "0.0";
        String drawHz = (hzText != null) ? hzText : "60Hz";

        float maxContentW = 0f;

        if (isHorizontal) {
            float row1W = measurePaintNum.measureText(drawFps) + measurePaintSub.measureText(" FPS  (" + drawFt + " ms) [" + drawHz + "]");

            StringBuilder sbChips = new StringBuilder();
            if (showCpu && cpuText != null) sbChips.append(cpuText);
            if (showGov && govText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(govText);
            if (showGpu && gpuText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(gpuText);
            if (showGpuGov && gpuGovText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append("GPUGov: ").append(gpuGovText);
            if (showRam && ramText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(ramText);
            if (showZram && zramText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(zramText);
            if (showBattery && pwrText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(pwrText);
            if (showNet && netText != null) sbChips.append(sbChips.length() > 0 ? " • " : "").append(netText);

            float row2W = measurePaintSub.measureText(sbChips.toString());
            maxContentW = Math.max(row1W, row2W);
        } else {
            maxContentW = Math.max(
                    measurePaintNum.measureText(drawFps + " FPS"),
                    measurePaintSub.measureText(drawFt + " ms [" + drawHz + "]")
            );

            if (showCpu && cpuText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText(cpuText));
            if (showGov && govText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText("CPUGov: " + govText));
            if (showGpu && gpuText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText(gpuText));
            if (showGpuGov && gpuGovText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText("GPUGov: " + gpuGovText));
            if (showRam && ramText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText(ramText));
            if (showZram && zramText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText(zramText));
            if (showBattery && pwrText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText(pwrText));
            if (showNet && netText != null) maxContentW = Math.max(maxContentW, measurePaintSub.measureText(netText));
        }

        // Add 28dp padding for left + right
        float paddingX = dpToPx(28) * scale;
        int requiredWidthPx = Math.round(maxContentW + paddingX);
        int targetWidthPx = Math.max((int)(dpToPx(bgWidthDp) * scale), requiredWidthPx);

        // Height calculation: Directly obey user bgHeightDp slider!
        int targetHeightPx = Math.max((int)(dpToPx(24) * scale), (int)(dpToPx(bgHeightDp) * scale));

        dimsResult[0] = targetWidthPx;
        dimsResult[1] = targetHeightPx;
        return dimsResult;
    }

    private static void createHudWindow() {
        hudView = new CanvasHudView(context);

        if (posY < 0) posY = 0;

        int[] dims = calcHudDimensions();
        int initialW = dims[0];
        int initialH = dims[1];

        params = new WindowManager.LayoutParams(
                initialW,
                initialH,
                2038, // TYPE_APPLICATION_OVERLAY
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                        | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                        | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                PixelFormat.TRANSLUCENT
        );
        params.gravity = Gravity.TOP | Gravity.START;
        params.x = posX;
        params.y = posY;
        params.setTitle("HyperMoonOverlay");
        params.packageName = "android";

        // Enable full-screen rendering across display cutouts & notches on AOSP
        try {
            java.lang.reflect.Field cutoutField = WindowManager.LayoutParams.class.getField("layoutInDisplayCutoutMode");
            cutoutField.setInt(params, 1); // LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
        } catch (Throwable ignored) {}

        hudView.setOnTouchListener(new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;
            private int cachedScreenW = 1080, cachedScreenH = 2400;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        isDragging = true;
                        initialX = params.x;
                        initialY = params.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        int[] screenSize = getRealScreenSize();
                        cachedScreenW = screenSize[0];
                        cachedScreenH = screenSize[1];
                        return true;

                    case MotionEvent.ACTION_MOVE:
                        int newX = initialX + (int) (event.getRawX() - initialTouchX);
                        int newY = initialY + (int) (event.getRawY() - initialTouchY);

                        int viewW = v.getWidth() > 0 ? v.getWidth() : params.width;
                        int viewH = v.getHeight() > 0 ? v.getHeight() : params.height;

                        int maxX = Math.max(0, cachedScreenW - Math.min(viewW, cachedScreenW / 2));
                        int maxY = Math.max(0, cachedScreenH - Math.min(viewH, cachedScreenH / 2));

                        if (newX < 0) newX = 0;
                        if (newY < 0) newY = 0;
                        if (newX > maxX) newX = maxX;
                        if (newY > maxY) newY = maxY;

                        if (Math.abs(newX - params.x) >= 2 || Math.abs(newY - params.y) >= 2) {
                            params.x = newX;
                            params.y = newY;
                            try {
                                windowManager.updateViewLayout(hudView, params);
                            } catch (Exception ignored) {}
                        }
                        return true;

                    case MotionEvent.ACTION_CANCEL:
                    case MotionEvent.ACTION_UP:
                        isDragging = false;
                        posX = params.x;
                        posY = params.y;
                        handler.post(new Runnable() {
                            @Override
                            public void run() {
                                savePosition();
                            }
                        });
                        return true;
                }
                return false;
            }
        });

        int[] windowTypes = new int[]{
            2038, // TYPE_APPLICATION_OVERLAY
            2010, // TYPE_SYSTEM_ERROR
            2006, // TYPE_SYSTEM_OVERLAY
            2003, // TYPE_SYSTEM_ALERT
            2032, // TYPE_ACCESSIBILITY_OVERLAY
            2034, // TYPE_SYSTEM_NOTIFICATION
            2015, // TYPE_SECURE_SYSTEM_OVERLAY
            2002  // TYPE_PHONE
        };

        boolean attached = false;
        boolean pidRejected = false;
        for (int type : windowTypes) {
            if (pidRejected) break;
            params.type = type;
            if (type == 2006 || type == 2015) {
                params.flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
            } else {
                params.flags &= ~WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
            }

            // Strategy 1: Create a dedicated WindowContext for this layer on Android 12+ (API 31+)
            Context targetCtx = null;
            WindowManager targetWm = null;

            if (Build.VERSION.SDK_INT >= 30 && defaultDisplay != null && sysContext != null) {
                try {
                    Method createWindowContextMethod = Context.class.getMethod("createWindowContext", Display.class, int.class, android.os.Bundle.class);
                    targetCtx = (Context) createWindowContextMethod.invoke(sysContext, defaultDisplay, type, null);
                } catch (Throwable t1) {
                    try {
                        Method createWindowContextMethod = Context.class.getMethod("createWindowContext", int.class, android.os.Bundle.class);
                        targetCtx = (Context) createWindowContextMethod.invoke(sysContext, type, null);
                    } catch (Throwable ignored) {}
                }
            }

            if (targetCtx != null) {
                try {
                    targetWm = (WindowManager) targetCtx.getSystemService(Context.WINDOW_SERVICE);
                } catch (Throwable ignored) {}
            }

            // Strategy 2: SystemContext root WindowManager (global, no WindowContext type lock)
            if (targetWm == null && sysContext != null) {
                try {
                    targetWm = (WindowManager) sysContext.getSystemService(Context.WINDOW_SERVICE);
                    targetCtx = sysContext;
                } catch (Throwable ignored) {}
            }

            if (targetWm == null) {
                targetWm = windowManager;
                targetCtx = context;
            }

            // Candidate packages to try
            String[] candidatePackages = new String[]{
                "android",
                "com.android.shell",
                (targetCtx != null ? targetCtx.getPackageName() : null),
                null
            };

            for (String pkg : candidatePackages) {
                params.packageName = pkg;
                try {
                    if (targetWm != null) {
                        targetWm.addView(hudView, params);
                        System.out.println("[HyperMoon] Successfully attached to WindowManager layer " + type + (pkg != null ? " (pkg=" + pkg + ")" : ""));
                        windowManager = targetWm;
                        context = targetCtx != null ? targetCtx : context;
                        attached = true;
                        break;
                    }
                } catch (Throwable t) {
                    String msg = (t.getMessage() != null) ? t.getMessage() : t.toString();
                    if (msg.contains("Window type mismatch") && sysContext != null) {
                        try {
                            WindowManager rootWm = (WindowManager) sysContext.getSystemService(Context.WINDOW_SERVICE);
                            if (rootWm != null && rootWm != targetWm) {
                                rootWm.addView(hudView, params);
                                System.out.println("[HyperMoon] Successfully attached to WindowManager layer " + type + " via SystemContext" + (pkg != null ? " (pkg=" + pkg + ")" : ""));
                                windowManager = rootWm;
                                context = sysContext;
                                attached = true;
                                break;
                            }
                        } catch (Throwable t2) {
                            msg = (t2.getMessage() != null) ? t2.getMessage() : t2.toString();
                        }
                    }
                    if (msg.contains("Unknown pid=") || msg.contains("unknown pid")) {
                        System.err.println("[HyperMoon Warning] WindowManager rejected root process: " + msg);
                        System.out.println("[HyperMoon] Android 14 QPR3+/15 process isolation detected. Bypassing WindowManager to Direct Hardware SurfaceControl...");
                        pidRejected = true;
                        break;
                    }
                    System.err.println("[HyperMoon Warning] Failed layer " + type + (pkg != null ? " [" + pkg + "]" : "") + ": " + msg);
                }
            }

            if (attached) break;
            if (isHardwareSurface) break;
        }

        if (!attached) {
            System.out.println("[HyperMoon] Initializing Direct Hardware SurfaceControl engine...");
            attached = initHardwareSurfaceControl(initialW, initialH);
        }

        if (!attached) {
            System.err.println("[HyperMoon ERROR] Unable to attach overlay via WindowManager or Hardware SurfaceControl!");
        } else {
            if (!isHardwareSurface && hudView != null) {
                try {
                    hudView.setVisibility(View.VISIBLE);
                    hudView.requestLayout();
                    hudView.invalidate();
                } catch (Throwable ignored) {}
            } else if (isHardwareSurface) {
                renderHardwareSurface();
            }
        }
    }

    private static boolean initHardwareSurfaceControl(int width, int height) {
        try {
            int w = Math.max(80, width);
            int h = Math.max(30, height);
            currentScW = w;
            currentScH = h;

            Class<?> scClass = Class.forName("android.view.SurfaceControl");
            Class<?> builderClass = Class.forName("android.view.SurfaceControl$Builder");
            Object builder = builderClass.getConstructor().newInstance();
            builderClass.getMethod("setName", String.class).invoke(builder, "HyperMoonHardwareOverlay");
            builderClass.getMethod("setBufferSize", int.class, int.class).invoke(builder, w, h);
            builderClass.getMethod("setFormat", int.class).invoke(builder, PixelFormat.TRANSLUCENT);

            try {
                builderClass.getMethod("setOpaque", boolean.class).invoke(builder, false);
            } catch (Throwable ignored) {}
            try {
                builderClass.getMethod("setHidden", boolean.class).invoke(builder, false);
            } catch (Throwable ignored) {}

            surfaceControl = builderClass.getMethod("build").invoke(builder);
            if (surfaceControl == null) return false;

            Class<?> txClass = Class.forName("android.view.SurfaceControl$Transaction");
            surfaceControlTx = txClass.getConstructor().newInstance();
            txSetPos = txClass.getMethod("setPosition", scClass, float.class, float.class);
            txSetVis = txClass.getMethod("setVisibility", scClass, boolean.class);
            txSetLayer = txClass.getMethod("setLayer", scClass, int.class);
            txApply = txClass.getMethod("apply");
            txRemove = txClass.getMethod("remove", scClass);

            try {
                txSetLayerStack = txClass.getMethod("setLayerStack", scClass, int.class);
            } catch (Throwable ignored) {}

            txSetLayer.invoke(surfaceControlTx, surfaceControl, 2147483640);
            if (txSetLayerStack != null) {
                try {
                    txSetLayerStack.invoke(surfaceControlTx, surfaceControl, 0);
                } catch (Throwable ignored) {}
            }
            txSetPos.invoke(surfaceControlTx, surfaceControl, (float) posX, (float) posY);
            txSetVis.invoke(surfaceControlTx, surfaceControl, isVisible);
            txApply.invoke(surfaceControlTx);

            Class<?> surfaceClass = Class.forName("android.view.Surface");
            hardwareSurface = surfaceClass.getConstructor(scClass).newInstance(surfaceControl);
            scLockCanvas = surfaceClass.getMethod("lockCanvas", Rect.class);
            scUnlockCanvasAndPost = surfaceClass.getMethod("unlockCanvasAndPost", Canvas.class);

            isHardwareSurface = true;
            System.out.println("[HyperMoon] Hardware SurfaceControl activated (" + w + "x" + h + " at " + posX + "," + posY + ")");
            return true;
        } catch (Throwable t) {
            System.err.println("[HyperMoon ERROR] Hardware SurfaceControl init failed: " + t.getMessage());
            isHardwareSurface = false;
            return false;
        }
    }

    private static void resizeHardwareSurface(int newW, int newH) {
        if (newW <= 0 || newH <= 0 || (newW == currentScW && newH == currentScH)) return;
        try {
            if (hardwareSurface != null) {
                try {
                    Method rel = hardwareSurface.getClass().getMethod("release");
                    rel.invoke(hardwareSurface);
                } catch (Throwable ignored) {}
            }
            if (surfaceControl != null && txRemove != null && surfaceControlTx != null) {
                try {
                    txRemove.invoke(surfaceControlTx, surfaceControl);
                    txApply.invoke(surfaceControlTx);
                } catch (Throwable ignored) {}
            }
            initHardwareSurfaceControl(newW, newH);
        } catch (Throwable t) {
            System.err.println("[HyperMoon] Hardware Surface resize error: " + t.getMessage());
        }
    }

    private static void renderHardwareSurface() {
        if (!isHardwareSurface || hardwareSurface == null || scLockCanvas == null || scUnlockCanvasAndPost == null) {
            return;
        }

        if (!isVisible) {
            try {
                if (surfaceControlTx != null && txSetVis != null) {
                    txSetVis.invoke(surfaceControlTx, surfaceControl, false);
                    txApply.invoke(surfaceControlTx);
                }
            } catch (Throwable ignored) {}
            return;
        }

        int[] dims = calcHudDimensions();
        int targetW = dims[0];
        int targetH = dims[1];
        if (targetW != currentScW || targetH != currentScH) {
            resizeHardwareSurface(targetW, targetH);
        }

        try {
            if (surfaceControlTx != null && txSetPos != null) {
                txSetPos.invoke(surfaceControlTx, surfaceControl, (float) posX, (float) posY);
                txSetVis.invoke(surfaceControlTx, surfaceControl, true);
                txApply.invoke(surfaceControlTx);
            }

            Canvas canvas = (Canvas) scLockCanvas.invoke(hardwareSurface, (Object) null);
            if (canvas != null) {
                try {
                    canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
                    drawHud(canvas, targetW, targetH);
                } finally {
                    scUnlockCanvasAndPost.invoke(hardwareSurface, canvas);
                }
            }
        } catch (Throwable t) {
            // Fail-safe protection against canvas rendering crashes
        }
    }

    private static int[] getRealScreenSize() {
        try {
            if (context != null) {
                DisplayManager dm = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
                if (dm != null) {
                    Display display = dm.getDisplay(Display.DEFAULT_DISPLAY);
                    if (display != null) {
                        DisplayMetrics realMetrics = new DisplayMetrics();
                        display.getRealMetrics(realMetrics);
                        return new int[]{ realMetrics.widthPixels, realMetrics.heightPixels };
                    }
                }
            }
        } catch (Throwable ignored) {}

        try {
            if (context != null && context.getResources() != null) {
                DisplayMetrics dm = context.getResources().getDisplayMetrics();
                if (dm != null) {
                    return new int[]{ dm.widthPixels, dm.heightPixels };
                }
            }
        } catch (Throwable ignored) {}

        return new int[]{ 1080, 2400 };
    }

    private static int dpToPx(int dp) {
        if (context == null) return Math.round(dp * 2.5f);
        try {
            DisplayManager dm = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
            if (dm != null) {
                Display display = dm.getDisplay(Display.DEFAULT_DISPLAY);
                if (display != null) {
                    DisplayMetrics realMetrics = new DisplayMetrics();
                    display.getRealMetrics(realMetrics);
                    return Math.round(dp * realMetrics.density);
                }
            }
        } catch (Throwable ignored) {}

        try {
            if (context.getResources() != null) {
                DisplayMetrics dm = context.getResources().getDisplayMetrics();
                if (dm != null) {
                    return Math.round(dp * dm.density);
                }
            }
        } catch (Throwable ignored) {}

        return Math.round(dp * 2.5f);
    }

    private static int spToPx(float sp) {
        if (context == null) return Math.round(sp * 2.5f);
        try {
            DisplayManager dm = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
            if (dm != null) {
                Display display = dm.getDisplay(Display.DEFAULT_DISPLAY);
                if (display != null) {
                    DisplayMetrics realMetrics = new DisplayMetrics();
                    display.getRealMetrics(realMetrics);
                    return Math.round(sp * realMetrics.scaledDensity);
                }
            }
        } catch (Throwable ignored) {}

        try {
            if (context.getResources() != null) {
                DisplayMetrics dm = context.getResources().getDisplayMetrics();
                if (dm != null) {
                    return Math.round(sp * dm.scaledDensity);
                }
            }
        } catch (Throwable ignored) {}

        return Math.round(sp * 2.5f);
    }

    private static void startLoop() {
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                try {
                    updateHudData();
                } catch (Throwable t) {
                    System.err.println("[HyperMoon] Recovered loop: " + t.getMessage());
                } finally {
                    try {
                        int delay = Math.max(50, refreshInterval);
                        handler.postDelayed(this, delay);
                    } catch (Throwable t2) {
                        try {
                            handler.postDelayed(this, 250);
                        } catch (Throwable ignored) {}
                    }
                }
            }
        }, Math.max(50, refreshInterval));
    }

    private static String getDynamicScreenHz() {
        try {
            if (context != null) {
                DisplayManager dm = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
                if (dm != null) {
                    Display display = dm.getDisplay(Display.DEFAULT_DISPLAY);
                    if (display != null) {
                        float refreshRate = display.getRefreshRate();
                        if (refreshRate <= 0) {
                            Display.Mode mode = display.getMode();
                            if (mode != null) {
                                refreshRate = mode.getRefreshRate();
                            }
                        }
                        if (refreshRate > 0) {
                            return Math.round(refreshRate) + "Hz";
                        }
                    }
                }
            }
        } catch (Throwable ignored) {}
        return null;
    }

    private static void updateHudData() {
        // Instant config sync when config file is modified
        File cfgFile = new File(configPath);
        if (cfgFile.exists()) {
            long modTime = cfgFile.lastModified();
            if (modTime != lastConfigModified) {
                readConfig();
                lastConfigModified = modTime;
            }
        }

        // Instant position sync when position file is modified
        File posFile = new File(posPath);
        if (posFile.exists()) {
            long pMod = posFile.lastModified();
            if (pMod != lastPosModified) {
                readPosition();
                lastPosModified = pMod;
            }
        }

        // Ensure overlay stays within current screen bounds when rotating or loading position
        int[] screenSize = getRealScreenSize();
        int screenW = screenSize[0];
        int screenH = screenSize[1];
        int viewW = isHardwareSurface ? currentScW : (hudView != null && hudView.getWidth() > 0 ? hudView.getWidth() : (params != null ? params.width : 200));
        int viewH = isHardwareSurface ? currentScH : (hudView != null && hudView.getHeight() > 0 ? hudView.getHeight() : (params != null ? params.height : 80));
        int maxX = Math.max(0, screenW - Math.min(viewW, screenW / 2));
        int maxY = Math.max(0, screenH - Math.min(viewH, screenH / 2));

        if (!isDragging) {
            boolean posAdjusted = false;
            if (posX > maxX) { posX = maxX; posAdjusted = true; }
            if (posY > maxY) { posY = maxY; posAdjusted = true; }
            if (posX < 0) { posX = 0; posAdjusted = true; }
            if (posY < 0) { posY = 0; posAdjusted = true; }

            if (posAdjusted && !isHardwareSurface && windowManager != null && hudView != null && params != null) {
                params.x = posX;
                params.y = posY;
                try {
                    windowManager.updateViewLayout(hudView, params);
                } catch (Exception ignored) {}
            }
        }

        Map<String, String> stats = readStats();

        if (!isVisible) {
            if (!isHardwareSurface && hudView != null) {
                hudView.setVisibility(View.GONE);
            } else if (isHardwareSurface) {
                renderHardwareSurface();
            }
            return;
        } else {
            if (!isHardwareSurface && hudView != null) {
                hudView.setVisibility(View.VISIBLE);
            }
        }

        fpsText = stats.getOrDefault("fps", "60");
        ftText  = stats.getOrDefault("frametime", "16.6");

        String hwHz = getDynamicScreenHz();
        if (hwHz != null && !hwHz.isEmpty()) {
            hzText = hwHz;
        } else {
            hzText = stats.getOrDefault("screen_hz", "60Hz");
        }

        String cTemp = stats.getOrDefault("cpu_temp", "--");
        String cLoad = stats.getOrDefault("cpu_load", "--");
        String cFreq = stats.getOrDefault("cpu_freq", "");
        if (showCpuFreq && !cFreq.isEmpty() && !"--".equals(cFreq) && !"0.00GHz".equals(cFreq)) {
            cpuText = "CPU " + cTemp + "°C (" + cLoad + "%) " + cFreq;
        } else {
            cpuText = "CPU " + cTemp + "°C (" + cLoad + "%)";
        }

        govText = stats.getOrDefault("cpu_gov", "schedutil");

        String gLoad = stats.getOrDefault("gpu_load", "--");
        String gTemp = stats.getOrDefault("gpu_temp", "--");
        String gFreq = stats.getOrDefault("gpu_freq", "");
        if (showGpuFreq && !gFreq.isEmpty() && !"--".equals(gFreq) && !"0MHz".equals(gFreq)) {
            gpuText = "GPU " + gTemp + "°C (" + gLoad + "%) " + gFreq;
        } else {
            gpuText = "GPU " + gTemp + "°C (" + gLoad + "%)";
        }

        gpuGovText = stats.getOrDefault("gpu_gov", "ged_dvfs");

        String rUsed  = stats.getOrDefault("ram_used",  "--");
        String rTotal = stats.getOrDefault("ram_total", "--");
        ramText = "RAM " + rUsed + "/" + rTotal + "GB";

        String zUsed  = stats.getOrDefault("swap_used",  "--");
        String zTotal = stats.getOrDefault("swap_total", "--");
        zramText = "ZRAM " + zUsed + "/" + zTotal + "GB";

        String watt = stats.getOrDefault("bat_watt", "--");
        String bTemp = stats.getOrDefault("bat_temp", "--");
        pwrText = "PWR " + watt + "W (" + bTemp + "°C)";

        String dl = stats.getOrDefault("net_dl", "0 KB/s");
        String ul = stats.getOrDefault("net_ul", "0 KB/s");
        netText = "DL " + dl + " • UL " + ul;

        if (!isHardwareSurface) {
            int[] dims = calcHudDimensions();
            int targetW = dims[0];
            int targetH = dims[1];

            if (params != null && (params.width != targetW || params.height != targetH)) {
                params.width  = targetW;
                params.height = targetH;
                if (hudView != null) hudView.requestLayout();
                try {
                    windowManager.updateViewLayout(hudView, params);
                } catch (Exception ignored) {}
            }

            if (hudView != null) hudView.invalidate();
        } else {
            renderHardwareSurface();
        }
    }

    private static Map<String, String> readStats() {
        Map<String, String> map = new HashMap<>();
        File file = new File(statePath);
        if (!file.exists()) return map;

        try (BufferedReader br = new BufferedReader(new FileReader(file))) {
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            String content = sb.toString();

            // Pre-compiled STATS_PATTERN matcher for zero allocation
            Matcher m = STATS_PATTERN.matcher(content);
            while (m.find()) {
                String key = m.group(1);
                String val = m.group(2) != null ? m.group(2) : m.group(3);
                if (key != null && val != null) {
                    map.put(key, val);
                }
            }
        } catch (Exception ignored) {}
        return map;
    }

    private static void readConfig() {
        File file = new File(configPath);
        if (!file.exists()) return;

        try (BufferedReader br = new BufferedReader(new FileReader(file))) {
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) {
                sb.append(line);
            }
            String content = sb.toString();

            autoGaming = parseBool(content, "auto_gaming", autoGaming);
            boolean baseVisible = parseBool(content, "visible", isVisible);
            if (autoGaming) {
                boolean isGaming = false;
                try {
                    File sf = new File("/data/adb/hypercore/status.json");
                    if (sf.exists()) {
                        BufferedReader sbr = new BufferedReader(new FileReader(sf));
                        String sline = sbr.readLine();
                        sbr.close();
                        if (sline != null && sline.contains("Gaming")) {
                            isGaming = true;
                        }
                    }
                } catch (Throwable ignored) {}
                isVisible = isGaming;
            } else {
                isVisible = baseVisible;
            }
            showFps = parseBool(content, "show_fps", showFps);
            showCpu = parseBool(content, "show_cpu", showCpu);
            showCpuFreq = parseBool(content, "show_cpu_freq", showCpuFreq);
            showGov = parseBool(content, "show_gov", showGov);
            showGpu = parseBool(content, "show_gpu", showGpu);
            showGpuFreq = parseBool(content, "show_gpu_freq", showGpuFreq);
            showGpuGov = parseBool(content, "show_gpu_gov", showGpuGov);
            showRam = parseBool(content, "show_ram", showRam);
            showZram = parseBool(content, "show_zram", showZram);
            showBattery = parseBool(content, "show_battery", showBattery);
            showNet = parseBool(content, "show_net", showNet);
            isHorizontal = parseBool(content, "is_horizontal", isHorizontal);
            align = parseStr(content, "align", align);
            theme = parseStr(content, "theme", theme);
            customHexColor = parseStr(content, "custom_color", customHexColor);
            opacity = parseFloat(content, "opacity", opacity);
            scale = parseFloat(content, "scale", scale);
            fontSizeSp = parseInt(content, "font_size", fontSizeSp);
            cornerRadiusDp = parseInt(content, "corner_radius", cornerRadiusDp);
            bgWidthDp = parseInt(content, "bg_width", bgWidthDp);
            bgHeightDp = parseInt(content, "bg_height", bgHeightDp);
            refreshInterval = parseInt(content, "refresh_interval", refreshInterval);
            if (refreshInterval < 50) refreshInterval = 50;
        } catch (Exception ignored) {}
    }

    // Flexible & Robust JSON Key-Value Extractors
    private static String getJsonRawVal(String json, String key) {
        if (json == null || key == null) return null;
        Pattern p = Pattern.compile("\"" + key + "\"\\s*:\\s*(?:\"([^\"]*)\"|([^,\\}\\]\\s]+))");
        Matcher m = p.matcher(json);
        if (m.find()) {
            String val = m.group(1) != null ? m.group(1) : m.group(2);
            return val != null ? val.trim() : null;
        }
        return null;
    }

    private static boolean parseBool(String json, String key, boolean defVal) {
        String val = getJsonRawVal(json, key);
        if (val != null) {
            if ("true".equalsIgnoreCase(val) || "1".equals(val)) return true;
            if ("false".equalsIgnoreCase(val) || "0".equals(val)) return false;
        }
        return defVal;
    }

    private static String parseStr(String json, String key, String defVal) {
        String val = getJsonRawVal(json, key);
        if (val != null && !val.isEmpty()) {
            return val;
        }
        return defVal;
    }

    private static float parseFloat(String json, String key, float defVal) {
        String val = getJsonRawVal(json, key);
        if (val != null) {
            try {
                val = val.replace(',', '.');
                return Float.parseFloat(val);
            } catch (Exception ignored) {}
        }
        return defVal;
    }

    private static int parseInt(String json, String key, int defVal) {
        String val = getJsonRawVal(json, key);
        if (val != null) {
            try {
                val = val.replace(',', '.');
                return Math.round(Float.parseFloat(val));
            } catch (Exception ignored) {}
        }
        return defVal;
    }

    private static void readPosition() {
        File file = new File(posPath);
        if (!file.exists()) return;

        try (BufferedReader br = new BufferedReader(new FileReader(file))) {
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
            String content = sb.toString();
            int newX = parseInt(content, "x", posX);
            int newY = parseInt(content, "y", posY);
            if (newX < 0) newX = 0;
            if (newY < 0) newY = 0;
            posX = newX;
            posY = newY;
        } catch (Exception ignored) {}
    }

    private static void savePosition() {
        File tmpFile = new File(posPath + ".tmp");
        File targetFile = new File(posPath);
        try (FileWriter fw = new FileWriter(tmpFile)) {
            fw.write("{\n  \"x\": " + posX + ",\n  \"y\": " + posY + "\n}\n");
            fw.flush();
            tmpFile.renameTo(targetFile);
        } catch (Exception ignored) {}
    }
}

