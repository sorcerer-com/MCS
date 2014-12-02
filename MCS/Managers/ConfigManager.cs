using System.IO;
using System.Xml;

namespace MCS.Managers
{
    public static class ConfigManager
    {
        private static string configFileName = "config.xml";

        private static XmlDocument xmlDoc;
        public static XmlDocument XmlDoc
        {
            get
            {
                if (xmlDoc == null)
                {
                    xmlDoc = new XmlDocument();
                    XmlElement xmlRoot = xmlDoc.CreateElement("Configs");
                    xmlDoc.AppendChild(xmlRoot);
                }
                return xmlDoc;
            }
        }


        public static void LoadConfig()
        {
            if (File.Exists(configFileName))
                XmlDoc.Load(configFileName);
            
            WindowsManager.LoadConfig(XmlDoc);
        }

        public static void SaveConfig()
        {
            XmlDoc.Save(configFileName);
        }
    }
}
