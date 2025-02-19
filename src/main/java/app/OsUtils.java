package app;

public class OsUtils {
/**
   * types of Operating Systems
   */
  public enum OSType {
    Windows, MacOS, Linux, Other
  };

  public static OSType getOperatingSystemType() {
    String OS = System.getProperty("os.name", "generic").toLowerCase();
    if ((OS.indexOf("mac") >= 0) || (OS.indexOf("darwin") >= 0)) {
      return OSType.MacOS;
    } else if (OS.indexOf("win") >= 0) {
      return OSType.Windows;
    } else if (OS.indexOf("nux") >= 0) {
      return OSType.Linux;
    } else {
      return OSType.Other;
    }
  }
}
