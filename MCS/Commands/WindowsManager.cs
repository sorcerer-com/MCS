using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace MCS.Commands
{
    public static class WindowsManager
    {
        private static Dictionary<string, Window> windows = new Dictionary<string, Window>();

        public static void ShowWindow(Type windowType)
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

            window = (Window)windowType.GetConstructor(Type.EmptyTypes).Invoke(null);

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
