using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Squirrel_trace_viewer
{
    public abstract class AElement
    {
        public abstract JTokenType Type { get; }

        public abstract string Description();

        public override string ToString() { return Description(); }

        public virtual string GetTreeLabel() => this.Description();

        public virtual void AddChildsToTree(ItemCollection items)
        { }

        public TreeViewItem ToTreeItem(string header)
        {
            TreeViewItem item = new TreeViewItem() { Header = header + ": " + this.GetTreeLabel() };
            this.AddChildsToTree(item.Items);
            return item;
        }

        public virtual void Load(Dictionary<UInt32, AElement> objects)
        { }

        public virtual bool Contains(string value) => this.Description().Contains(value);

        public static AElement Create(JToken obj)
        {
            switch (obj.Type)
            {
                case JTokenType.Null:
                    return new Null();

                case JTokenType.Boolean:
                    return new Boolean(obj);

                case JTokenType.Integer:
                    return new Integer(obj);

                case JTokenType.Float:
                    return new Float(obj);

                case JTokenType.String:
                    if (AObject.IsAddr((string)obj))
                        return new Reference(obj);
                    else
                        return new String(obj);

                case JTokenType.Object:
                    JToken objectType = obj["ObjectType"];
                    if (objectType != null && objectType.Type == JTokenType.String && (string)objectType == "SQTable")
                        return new SQTable(obj);
                    return new JsonObject(obj);

                case JTokenType.Array:
                    return new JsonArray(obj);

                default:
                    return new String(obj.ToString());
            }
        }
    }

    class Instruction : AElement
    {
        public string fn { get; }
        public string op { get; }
        public AElement arg0 { get; }
        public AElement arg1 { get; }
        public AElement arg2 { get; }
        public AElement arg3 { get; }

        public Instruction(JObject obj)
        {
            fn = (string)obj["fn"];
            op = (string)obj["op"];
            arg0 = AElement.Create(obj["arg0"]);
            arg1 = AElement.Create(obj["arg1"]);
            arg2 = AElement.Create(obj["arg2"]);
            arg3 = AElement.Create(obj["arg3"]);
        }

        public override JTokenType Type => JTokenType.None;
        public override string Description() => null;

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            arg0.Load(objects);
            arg1.Load(objects);
            arg2.Load(objects);
            arg3.Load(objects);
        }

        public override bool Contains(string value)
        {
            return this.fn.Contains(value) ||
                this.op.Contains(value) ||
                this.arg0.Contains(value) ||
                this.arg1.Contains(value) ||
                this.arg2.Contains(value) ||
                this.arg3.Contains(value);
        }

        public override void AddChildsToTree(ItemCollection items)
        {
            items.Add(new TreeViewItem() { Header = "op: " + op });
            items.Add(arg0.ToTreeItem("arg0"));
            items.Add(arg1.ToTreeItem("arg1"));
            items.Add(arg2.ToTreeItem("arg2"));
            items.Add(arg3.ToTreeItem("arg3"));
        }
    }

    class Null : AElement
    {
        public Null()
        { }

        public override JTokenType Type => JTokenType.Null;
        public override string Description() => "null";
        public override bool Contains(string value) => false;
    }

    class Boolean : AElement
    {
        bool value;

        public Boolean(JToken obj)
        {
            value = (bool)obj;
        }

        public override JTokenType Type => JTokenType.Boolean;
        public override string Description() => value.ToString();
    }

    class Integer : AElement
    {
        long value;

        public Integer(long n)
        {
            value = n;
        }

        public Integer(JToken obj)
        {
            value = (long)obj;
        }

        public override JTokenType Type => JTokenType.Integer;
        public override string Description() => value.ToString();
    }

    class Float : AElement
    {
        float value;

        public Float(JToken obj)
        {
            value = (float)obj;
        }

        public override JTokenType Type => JTokenType.Float;
        public override string Description() => value.ToString();
    }

    class String : AElement
    {
        string value;

        public String(JToken obj)
        {
            value = (string)obj;
        }

        public String(string str)
        {
            value = str;
        }

        public override JTokenType Type => JTokenType.String;
        public override string Description() => '"' + value + '"';

        public bool Compare(string other) => this.value == other;
    }

    abstract class AObject : AElement
    {
        const string head = "POINTER:";

        public static bool IsAddr(string str)
        {
            return str.Length > head.Length && str.Substring(0, head.Length).Equals(head);
        }

        public static UInt32 StrToAddr(string str)
        {
            if (IsAddr(str) == false)
                return 0;
            str = str.Substring(head.Length);
            return UInt32.Parse(str, System.Globalization.NumberStyles.HexNumber);
        }
    }

    class JsonObject : AObject
    {
        protected Dictionary<AElement, AElement> elems;
        protected string ObjectType;

        public JsonObject()
        {
            elems = new Dictionary<AElement, AElement>();
            ObjectType = "Unknown type";
        }

        public JsonObject(JToken obj)
        {
            elems = new Dictionary<AElement, AElement>();
            ObjectType = "Unknown type";
            foreach (var it in (JObject)obj)
            {
                if (it.Key == "ObjectType")
                    ObjectType = (string)it.Value;
                else
                    elems[AElement.Create(it.Key)] = AElement.Create(it.Value);
            }
        }

        public override JTokenType Type => JTokenType.Object;
        public override string Description() => "{...}";

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            foreach (var it in elems)
            {
                it.Key.Load(objects);
                it.Value.Load(objects);
            }
        }

        public AElement this[string key]
        {
            get {
                return this.elems.Single(x => {
                    if (x.Key.Type != JTokenType.String)
                        return false;
                    return x.Key.ToString() == '"' + key + '"';
                    }).Value;
            }
        }

        public override string GetTreeLabel() => this.ObjectType;

        public override void AddChildsToTree(ItemCollection items)
        {
            foreach (var it in elems)
            {
                TreeViewItem item = new TreeViewItem() { Header = it.Key };
                it.Value.AddChildsToTree(item.Items);
                if (item.Items.Count == 0)
                    item.Header += ": " + it.Value.Description();
                items.Add(item);
            }
        }

        public override bool Contains(string value)
        {
            foreach (var it in elems)
                if (it.Value.Contains(value))
                    return true;
            return false;
        }
    }

    class SQTable : JsonObject
    {
        public SQTable(JToken obj)
            : base(obj)
        { }

        public override void AddChildsToTree(ItemCollection items)
        {
            JsonArray nodes = this["_nodes"] as JsonArray;
            foreach (AElement it in nodes)
            {
                AElement key = (it as JsonObject)["key"];
                AElement val = (it as JsonObject)["val"];
                if (key.Type == JTokenType.String || key.Type == JTokenType.Null)
                    items.Add(val.ToTreeItem(key.GetTreeLabel()));
                else
                {
                    items.Add(key.ToTreeItem("key"));
                    items.Add(val.ToTreeItem("val"));
                }
            }
        }
    }

    class Reference : AObject
    {
        UInt32 addr;
        AElement target;
        bool isRecursing;

        public Reference(JToken obj)
        {
            addr = AObject.StrToAddr((string)obj);
            target = null;
            isRecursing = false;
        }

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            if (target != null)
                return;

            if (objects.TryGetValue(addr, out target))
                target.Load(objects);
            else
            {
                Console.WriteLine("Unknown object at address " + addr);
                target = null;
            }
        }

        public override bool Contains(string value)
        {
            if (target == null)
                return false;
            return target.Contains(value);
        }

        public override string Description()
        {
            if (target == null)
                return "<ref not loaded>";
            return target.Description();
        }

        public override JTokenType Type {
            get {
                if (target == null)
                    return JTokenType.None;
                return target.Type;
            }
        }

        public override string GetTreeLabel()
        {
            if (target == null)
                return "<ref not loaded>";
            return target.GetTreeLabel();
        }

        public override void AddChildsToTree(ItemCollection items)
        {
            if (target == null)
            {
                items.Add(new TreeViewItem() { Header = "<ref not loaded>" });
                return;
            }
            if (isRecursing)
                return; // Infinite recursion
            isRecursing = true;
            target.AddChildsToTree(items);
            isRecursing = false;
        }
    }

    class JsonArray : AObject, IEnumerable
    {
        List<AElement> elems;

        public JsonArray(JToken obj)
        {
            elems = new List<AElement>();
            foreach (var it in (JArray)obj)
            {
                elems.Add(AElement.Create(it));
            }
        }

        public override JTokenType Type => JTokenType.Array;
        public override string Description() => "[...]";

        public IEnumerator GetEnumerator()
        {
            foreach (var it in elems)
                yield return it;
        }

        public override void Load(Dictionary<UInt32, AElement> objects)
        {
            foreach (AElement it in elems)
                it.Load(objects);
        }

        public override bool Contains(string value)
        {
            foreach (var it in elems)
                if (it.Contains(value))
                    return true;
            return false;
        }

        public override string GetTreeLabel() => "Array";

        public override void AddChildsToTree(ItemCollection items)
        {
            int i = 0;
            foreach (AElement it in elems)
            {
                TreeViewItem item = new TreeViewItem() { Header = it.GetTreeLabel() };
                if ((string)item.Header == "Unknown type")
                    item.Header = i;
                it.AddChildsToTree(item.Items);
                items.Add(item);
                i++;
            }
        }
    }
}
