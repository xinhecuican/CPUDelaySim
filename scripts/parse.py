import ast
import os
import argparse
import xml.etree.ElementTree as ET
import fnmatch

def analyze_ast(filename):
    classes_info = []
    imports = []
    # 类名到属性的映射，用于解析类属性引用
    class_attributes_map = {}
    
    with open(filename, 'r') as file:
        source = file.read()
        tree = ast.parse(source)
        
        # 第一遍：收集所有类的简单属性（不依赖其他类的属性）和导入信息
        for node in ast.walk(tree):
            # 处理导入语句
            if isinstance(node, ast.Import):
                for name in node.names:
                    imports.append({
                        'type': 'import',
                        'module': name.name,
                        'alias': name.asname
                    })
            elif isinstance(node, ast.ImportFrom):
                for name in node.names:
                    imports.append({
                        'type': 'from_import',
                        'module': node.module,
                        'name': name.name,
                        'alias': name.asname,
                        'level': node.level  # 相对导入级别
                    })
            # 处理类定义
            elif isinstance(node, ast.ClassDef):
                class_attributes_map[node.name] = {}
                for item in node.body:
                    if isinstance(item, ast.Assign):
                        for target in item.targets:
                            if isinstance(target, ast.Name):
                                # 尝试获取变量值
                                try:
                                    value = ast.literal_eval(item.value)
                                    class_attributes_map[node.name][target.id] = value
                                except:
                                    # 暂时跳过需要引用其他类的属性
                                    pass
        
        # 第二遍：处理所有类，包括解析引用其他类的属性
        for node in ast.walk(tree):
            if isinstance(node, ast.ClassDef):
                classInfo = {
                    'name': node.name,
                    'bases': [base.id for base in node.bases if isinstance(base, ast.Name)],
                    'methods': [],
                    'attributes': {},
                    'imports': imports.copy()  # 将收集到的导入信息添加到类信息中
                }

                for item in node.body:
                    if isinstance(item, ast.Assign):
                        for target in item.targets:
                            if isinstance(target, ast.Name):
                                # 尝试获取变量值
                                try:
                                    value = ast.literal_eval(item.value)
                                    classInfo['attributes'][target.id] = value
                                except:
                                    # 检查是否是类属性引用（形如 Class.attr）
                                    if isinstance(item.value, ast.Attribute) and isinstance(item.value.value, ast.Name):
                                        class_name = item.value.value.id
                                        attr_name = item.value.attr
                                        # 如果引用的类和属性存在，使用引用的值
                                        if class_name in class_attributes_map and attr_name in class_attributes_map[class_name]:
                                            classInfo['attributes'][target.id] = class_attributes_map[class_name][attr_name]
                                        else:
                                            # 否则保存表达式字符串
                                            classInfo['attributes'][target.id] = ast.unparse(item.value)
                                    else:
                                        # 其他情况保存表达式字符串
                                        classInfo['attributes'][target.id] = ast.unparse(item.value)
                classes_info.append(classInfo)
    return classes_info

cpuname = 'AtomicCPU'

def analyze_layer(filename):
    tree = ET.parse(filename)
    root = tree.getroot()
    
    # 存储类的层次结构和container信息
    class_hierarchy = {}
    
    # 递归遍历XML树，提取类名、container和type信息
    def traverse_xml(node):
        parent_name = node.tag
        for child in node:
            # 获取类名
            class_name = child.tag
            
            # 获取container和type属性
            container = child.get('container')
            type_attr = child.get('type')
            
            # 存储信息
            if parent_name not in class_hierarchy:
                class_hierarchy[parent_name] = []
            
            class_hierarchy[parent_name].append({
                'name': class_name,
                'container': container,
                'type': type_attr
            })
            
        # 递归处理子节点
        for child in node:
            traverse_xml(child)
    
    for ele in root:
        if 'CPU' in ele.tag:
            cpuname = ele.tag
            traverse_xml(ele)
            break
    cache = root.find("CacheManager")
    if cache is not None:
        traverse_xml(cache)


    return class_hierarchy


def main():
    args = argparse.ArgumentParser()
    args.add_argument("--filepath", type=str, default="")
    args = args.parse_args()
    current_dir = args.filepath
    obj_dir = os.path.join(current_dir, "obj")
    classes_info = []
    if not os.path.exists(obj_dir):
        os.makedirs(obj_dir)
    for file_name in os.listdir(current_dir):
        file_path = os.path.join(current_dir, file_name)
        name_ext = os.path.splitext(file_name)[1]
        if os.path.isfile(file_path) and name_ext == ".py":
            classes = analyze_ast(file_path)
            classes_info.extend(classes)
    
    classes = analyze_ast("configs/params.py")
    classes_info.extend(classes)

    # 通过后序遍历将父类的attributes合并到子类
    # 首先构建类名到类信息的映射
    class_map = {cls['name']: cls for cls in classes}
    
    # 所有类的attributes字典
    all_attributes = {}
    
    # 递归函数：获取类及其父类的所有属性
    def get_inherited_attributes(class_name):
        if class_name not in class_map:
            return {}
        
        # 先获取所有父类的属性（后序遍历）
        inherited_attrs = {}
        for base in class_map[class_name]['bases']:
            base_attrs = get_inherited_attributes(base)
            inherited_attrs.update(base_attrs)
        
        # 然后添加自己的属性，覆盖父类同名属性
        class_attrs = class_map[class_name]['attributes'].copy()
        inherited_attrs.update(class_attrs)
        
        # 将合并后的属性保存到all_attributes字典
        if class_name not in all_attributes:
            all_attributes[class_name] = inherited_attrs
        
        return inherited_attrs
    
    # 对每个类执行后序遍历获取继承属性
    for cls in classes:
        get_inherited_attributes(cls['name'])
    
    for cls in classes:
        name_no_ext = cls['name']
        file_h = os.path.join(obj_dir, "params_" + name_no_ext + ".h")
        with open(file_h, 'w') as f:
            f.write(f"#ifndef PARAMS_{name_no_ext.upper()}_H\n")
            f.write(f"#define PARAMS_{name_no_ext.upper()}_H\n\n")
            cls_attr = all_attributes[cls['name']]
            for attr, value in cls_attr.items():
                if attr == 'cxx_header':
                    continue
                if type(value) == int:
                    f.write(f"static constexpr uint64_t {cls['name']}_{attr} = {value};\n")
                elif type(value) == str:
                    f.write(f"static const std::string {cls['name']}_{attr} = \"{value}\";\n")
                elif type(value) == float:
                    f.write(f"static constexpr double {cls['name']}_{attr} = {value};\n")
                elif type(value) == bool:
                    f.write(f"static constexpr bool {cls['name']}_{attr} = {str(value).lower()};")
                elif type(value) == list:
                    f.write(f"static constexpr int {cls['name']}_{attr}[] = {{ {', '.join([str(val) for val in value])} }};\n")
            f.write("\n")
            f.write(f"#define {cls['name']}_SET_PARAMS \\\n")
            for attr, value in cls_attr.items():
                if attr == 'cxx_header':
                    continue
                if type(value) == list:
                    f.write(f"    {attr} = {{ {', '.join([str(val) for val in value])} }};\\\n")
                else:
                    f.write(f"    {attr} = {cls['name']}_{attr};\\\n")
            f.write("\n")
            f.write(f"#endif // PARAMS_{name_no_ext.upper()}_H\n")

    layers = analyze_layer(os.path.join(current_dir, "layer.xml"))
    with open(os.path.join(obj_dir, "params_EMU.h"), 'w') as f:
        f.write(f"#ifndef PARAMS_EMU_H\n")
        f.write(f"#define PARAMS_EMU_H\n\n")
        f.write(f"static constexpr std::string CPU_NAME = \"{cpuname}\";\n")
        f.write(f"#endif // PARAMS_EMU_H\n")
    current_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(current_dir)
    inc_dir = os.path.join(project_dir, "inc")
    
    # 函数：在inc目录下搜索头文件并返回相对路径
    def find_header_path(inc_dir, header_name):
        for root, dirs, files in os.walk(inc_dir):
            if header_name in files:
                # 获取从inc目录开始的相对路径
                relative_path = os.path.relpath(os.path.join(root, header_name), inc_dir)
                # 将路径分隔符从系统分隔符替换为斜杠
                return relative_path.replace(os.path.sep, '/')
        # 如果未找到，返回原始名称
        return header_name

    name_header_map = {}
    for cls in classes_info:
        name_header_map[cls['name']] = cls['attributes']['cxx_header']
    
    for cls in classes_info:
        name_no_ext = cls['name']
        header_name = name_no_ext.lower() + ".h"
        # 搜索头文件路径
        header_path = find_header_path(inc_dir, header_name)
        file_cpp = os.path.join(obj_dir, name_no_ext + "_load.cpp")
        with open(file_cpp, 'w') as f:
            f.write(f"#include \"{cls['attributes']['cxx_header']}\"\n")
            f.write(f"#include \"params_{name_no_ext}.h\"\n")
            if name_no_ext in layers:
                for child in layers[name_no_ext]:
                    if child['name'] in name_header_map:
                        f.write(f"#include \"{name_header_map[child['name']]}\"\n")
            f.write(f"void {cls['name']}::load() {{\n")
            f.write(f"    {cls['name']}_SET_PARAMS\n")
            if name_no_ext in layers:
                i = 0
                for child in layers[name_no_ext]:
                    if child['container'] is not None:
                        if child['type'] is not None:
                            f.write(f"    {child['name']}* {child['name']}_obj{i} = new {child['name']};\n")
                            f.write(f"    {child['name']}_obj{i}->load();\n")
                            f.write(f"    {child['container']}.push_back({child['name']}_obj{i});\n")
                            i += 1
                        else:
                            f.write(f"    {child['container']} = new {child['name']}();\n")
                            f.write(f"    {child['container']}->load();\n")
                layers.pop(name_no_ext)
            f.write("}\n")

    for key, value in layers.items():
        header_path = find_header_path(inc_dir, key.lower() + ".h")
        file_cpp = os.path.join(obj_dir, key + "_load.cpp")
        with open(file_cpp, 'w') as f:
            f.write(f"#include \"{header_path}\"\n")
            f.write(f"#include \"params_{key}.h\"\n")
            for child in value:
                if child['name'] in name_header_map:
                    f.write(f"#include \"{name_header_map[child['name']]}\"\n")
            f.write(f"void {key}::load() {{\n")
            i = 0
            for child in value:
                if child['container'] is not None:
                    if child['type'] is not None:
                        f.write(f"    {child['name']}* {child['name']}_obj{i} = new {child['name']};\n")
                        f.write(f"    {child['name']}_obj{i}->load();\n")
                        f.write(f"    {child['container']}.push_back({child['name']}_obj{i});\n")
                        i += 1
                    else:
                        f.write(f"    {child['container']} = new {child['name']}();\n")
                        f.write(f"    {child['container']}->load();\n")
            f.write("}\n")

if __name__ == "__main__":
    main()