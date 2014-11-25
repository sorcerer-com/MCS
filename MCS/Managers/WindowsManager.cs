using System;
using System.Collections.Generic;
using System.Windows;

namespace MCS.Managers
{
    public static class WindowsManager
    {
        private static Dictionary<string, Window> windows = new Dictionary<string, Window>();

        public static void ShowWindow(Type windowType, Object param)
        {
            if (!windowType.IsSubclassOf(typeof(Window)))
                return;

            if (!windows.ContainsKey(windowType.Name))
                windows.Add(windowType.Name, null);

            Window window = windows[windowType.Name];
            if (window != null && window.IsVisible)
            {
                window.Activate();
                return;
            }
            else if (window != null)
                window.Close();

            try
            {
                if (param != null)
                    window = windowType.GetConstructor(new Type[] { param.GetType() }).Invoke(new object[] { param }) as Window;
                else
                    window = windowType.GetConstructor(Type.EmptyTypes).Invoke(null) as Window;
            }
            catch { }
            if (window == null)
                throw new Exception("Cannot construct window from type: " + windowType + " with parametar: " + param);

            window.Show();
            windows[windowType.Name] = window;
        }

        public static void Close()
        {
            foreach (Window window in windows.Values)
            {
                if (window != null)
                    window.Close();
            }
        }
    }
}
