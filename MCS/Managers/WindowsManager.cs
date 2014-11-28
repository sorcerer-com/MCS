using MCS.MainWindows;
using MEngine;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Input;
using System.Xml;

namespace MCS.Managers
{
    public static class WindowsManager
    {
        private static string configFileName = "config.xml";

        private static Dictionary<string, Window> windows = new Dictionary<string, Window>();
        private static Dictionary<Type, Dictionary<Key, string>> hotkeys = new Dictionary<Type, Dictionary<Key, string>>(); // window type / (key / command name)


        public static void Init()
        {
            if (!loadHotkeysConfig())
            {
                defaultHotkeys();
                saveHotkeys();
            }
        }


        public static void ShowWindow(Type windowType, Object param = null)
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

            window.Owner = Application.Current.MainWindow;
            window.KeyDown += Window_KeyDown;
            window.Closing += (sender, e) => { window.KeyDown -= Window_KeyDown; };

            window.Show();
            windows[windowType.Name] = window;
        }


        // Hot keys functionality
        // TODO: key combinations, focus specified command
        private static bool loadHotkeysConfig()
        {
            if (!File.Exists(configFileName))
                return false;

            XmlDocument xmlDoc = new XmlDocument();
            xmlDoc.Load(configFileName);

            XmlNodeList xmlNodes = xmlDoc.GetElementsByTagName("Hotkeys");
            if (xmlNodes.Count != 1)
            {
                MScene.Log(ELogType.Warning, "Editor", "Hotkeys cannot be loaded from config file");
                return false;
            }
            
            XmlNode xmlRoot = xmlNodes.Item(0);
            for (int i = 0; i < xmlRoot.ChildNodes.Count; i++)
            {
                XmlNode xmlType = xmlRoot.ChildNodes.Item(i);
                Dictionary<Key, string> keys = new Dictionary<Key, string>();

                for (int j = 0; j < xmlType.ChildNodes.Count; j++)
                {
                    XmlNode xmlKey = xmlType.ChildNodes.Item(j);
                    string key = xmlKey.Attributes.GetNamedItem("Key").Value;
                    string command = xmlKey.Attributes.GetNamedItem("Command").Value;
                    keys.Add((Key)Enum.Parse(typeof(Key), key), command);
                }

                hotkeys.Add(Type.GetType(xmlType.Name), keys);
            }

            return true;
        }

        private static void defaultHotkeys()
        {
            Dictionary<Key, string> keys = null;

            // Main window
            keys = new Dictionary<Key, string>();
            keys.Add(Key.F8, "LogWindowCommand");
            keys.Add(Key.F3, "ContentWindowCommand");
            hotkeys.Add(typeof(MainWindow), keys);

            // Content window
            keys = new Dictionary<Key, string>();
            keys.Add(Key.F3, "CloneElementCommand");
            keys.Add(Key.F2, "RenameElementCommand");
            keys.Add(Key.F4, "MoveElementCommand");
            keys.Add(Key.Delete, "DeleteElementCommand");
            keys.Add(Key.F9, "ExportElementCommand");
            hotkeys.Add(typeof(ContentWindow), keys);
        }

        private static void saveHotkeys()
        {
            // TODO: change only hotkeys element of the config
            XmlDocument xmlDoc = new XmlDocument();
            XmlElement xmlRoot = xmlDoc.CreateElement("Hotkeys");
            xmlDoc.AppendChild(xmlRoot);

            foreach(var pair in hotkeys)
            {
                XmlElement xmlType = xmlDoc.CreateElement(pair.Key.FullName);
                xmlRoot.AppendChild(xmlType);

                foreach(var pair2 in pair.Value)
                {
                    XmlElement xmlKey = xmlDoc.CreateElement("Hotkey");
                    xmlKey.SetAttribute("Key", pair2.Key.ToString());
                    xmlKey.SetAttribute("Command", pair2.Value);
                    xmlType.AppendChild(xmlKey);
                }
            }

            xmlDoc.Save(configFileName);
        }

        public static void Window_KeyDown(object sender, KeyEventArgs e)
        {
            Type type = sender.GetType();
            if (!hotkeys.ContainsKey(type))
                return;

            Dictionary<Key, string> keys = hotkeys[type];
            if (!keys.ContainsKey(e.Key))
                return;

            string commandName = keys[e.Key];
            PropertyInfo pi = type.GetProperty(commandName);
            if (pi == null)
            {
                MScene.Log(ELogType.Warning, "Editor", "Try to execute invalid command '" + commandName + "' associate with hotkey '" + e.Key + "'");
                return;
            }

            ICommand command = pi.GetValue(sender) as ICommand;
            if (command == null)
            {
                MScene.Log(ELogType.Warning, "Editor", "Try to execute invalid command '" + commandName + "' associate with hotkey '" + e.Key + "'");
                return;
            }

            command.Execute(null);
        }

    }
}
