package capture;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;

public class Logger {

    private BufferedWriter maliciousLog;
    private BufferedWriter safeLog;
    private BufferedWriter progressLog;
    private BufferedWriter errorLog;
    private BufferedWriter statsLog;

    Thread statsThread = null;

    private Logger() {
        try {
            File f = new File("log");
            if (!f.isDirectory()) {
                f.mkdir();
            }

            maliciousLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + "malicious.log"), "UTF-8"));
            safeLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + "safe.log"), "UTF-8"));
            progressLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + "progress.log"), "UTF-8"));
            errorLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + "error.log"), "UTF-8"));
            statsLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + "stats.log"), "UTF-8"));

            writeToStatsLog();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    private void writeToStatsLog() {
        statsThread = new Thread(new Runnable() {
            public void run() {
                try {
                    String header = Stats.getHeader();
                    statsLog.write(header);
                    statsLog.newLine();
                    statsLog.flush();
                    while (true) {
                        String stats = Stats.getStats();
                        statsLog.write(stats);
                        statsLog.newLine();
                        statsLog.flush();

                        try {
                            Thread.sleep(1000 * 60 * 5);   //5 min waiting
                        } catch (InterruptedException e) {
                            System.out.println("Stats thread interrupted.");
                        }
                    }
                } catch (IOException e) {
                    System.out.println("Failed writing to stats log.");
                    e.printStackTrace(System.out);
                }
            }
        }, "statsThread");
        statsThread.start();
    }


    private static class LoggerHolder {
        private final static Logger instance = new Logger();
    }

    public static Logger getInstance() {
        return LoggerHolder.instance;
    }

    public void writeToMaliciousUrlLog(String text) {
        try {
            maliciousLog.write(text);
            maliciousLog.newLine();
            maliciousLog.flush();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public void writeToSafeUrlLog(String text) {
        try {
            safeLog.write(text);
            safeLog.newLine();
            safeLog.flush();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public void writeToProgressLog(String text) {
        try {
            progressLog.write(text);
            progressLog.newLine();
            progressLog.flush();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public void writeToErrorLog(String text) {
        try {
            errorLog.write(text);
            errorLog.newLine();
            errorLog.flush();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }
}
