#include "sylar/config.h"
#include "sylar/log.h"
#include "yaml-cpp/yaml.h"


sylar::ConfigVar<int>::ptr g_int_value_config = 
    sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config = 
    sylar::Config::Lookup("system.value", (float)10.2, "system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
sylar::Config::Lookup("system.int_vec", std::vector<int>{ 1, 2 }, "system int vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config = 
sylar::Config::Lookup("system.int_list", std::list<int>{ 1, 2 }, "system int list");

sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config = 
sylar::Config::Lookup("system.int_set", std::set<int>{ 1, 2 }, "system int set");

sylar::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    sylar::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

sylar::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    sylar::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    sylar::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");

void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
            << "NULL - " << node.Type() << " - " << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                    << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}


void test_config() {
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->toString();

    // auto v = g_int_vec_value_config->getValue();
    // for (auto i : v){
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before_int_vec: " << i;
    // }

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->toString(); \
    }


    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);


    //解析log.yml
    YAML::Node root = YAML::LoadFile("/home/hzh/workspace/high-performance-server/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    // v = g_int_vec_value_config->getValue();
    // for (auto i : v){
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after_int_vec: " << i;
    // }

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}



void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/hzh/workspace/high-performance-server/bin/conf/log.yml");
    print_yaml(root, 0);
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}


// test class
class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 2;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};


namespace sylar {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}
sylar::ConfigVar<Person>::ptr g_person =
    sylar::Config::Lookup("class.person", Person(), "class person");

sylar::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    sylar::Config::Lookup("class.map", std::map<std::string, Person>(), "class person");

sylar::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    sylar::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "class person");

void test_class() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

    XX_PM(g_person_map, "class.map before");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    g_person->addListener([](const Person& old_value, const Person& new_value){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new_value=" << new_value.toString();
    });

    YAML::Node root = YAML::LoadFile("/home/hzh/workspace/high-performance-server/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);



    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_person_vec_map->toString();
}

void test_log() {
    static sylar::Logger::ptr system_log = SYLAR_LOG_NAME("system");
    static sylar::Logger::ptr root_log = SYLAR_LOG_NAME("root");
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    SYLAR_LOG_INFO(system_log) << "hello system system_log old" << std::endl;
    YAML::Node root = YAML::LoadFile("/home/hzh/workspace/high-performance-server/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    std::cout << "=============" << std::endl;
    std::cout << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << "root: " << root << std::endl;

    SYLAR_LOG_INFO(system_log);
    SYLAR_LOG_INFO(root_log);
    system_log->setFormatter("%d ---------------- %d%n");
    //SYLAR_LOG_INFO(root_log) << "hello root root_log new" << std::endl;
    SYLAR_LOG_INFO(system_log);
}


int main(int argc, char** argv){

    //test_config();
    //test_yaml();
    //test_class();
    test_log();
    return 0;
}