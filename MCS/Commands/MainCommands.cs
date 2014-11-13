using MCS.MainWindows;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Input;

namespace MCS.Commands
{
    public class MainCommands
    {
        
        public ICommand LogWindowCommand
        {
            get { return new DelegateCommand((o) => { showWindow(typeof(LogWindow)); }); }
        }


        public ICommand ContentWindowCommand
        {
            get { return new DelegateCommand((o) => { showWindow(typeof(ContentWindow)); }); }
        }

        
        private Dictionary<string, Window> windows = new Dictionary<string, Window>();

        private void showWindow(Type windowType)
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
    }
}
