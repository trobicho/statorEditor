#pragma once

#include "statorNode.hpp"
#include <imgui.h>

class FactoryNode: public StatorNode {
  public:
    FactoryNode() {};

    template<typename T, typename... Params>
    std::shared_ptr<T>  placeNode(Params&&... args) {
      std::shared_ptr<T>  node = m_grid.placeNode<T>(args...);
      return (node);
    }
    template<typename T, typename... Params>
    std::shared_ptr<T>  placeNodeAt(const ImVec2& pos, Params&&... args) {
      std::shared_ptr<T>  node = m_grid.placeNodeAt<T>(pos, args...);
      return (node);
    }
    template<typename T, typename... Params>
    std::shared_ptr<T>  addNode(const ImVec2& pos, Params&&... args) {
      std::shared_ptr<T>  node = m_grid.addNode<T>(pos, args...);
      return (node);
    }

    StatorNodeType  statorNodeType() override {
      return (SNT_FACTORY_NODE);
    };

    json::value     toJson() override {
      json::array   nodesJson;
      auto          nodes = m_grid.getNodes();
      for (auto& nodePair: nodes) {
        auto node = std::dynamic_pointer_cast<StatorNode>(nodePair.second);
        ImVec2  pos = node->getPos();
        nodesJson.push_back(json::value{
          {"pos", {
            {"x", pos.x},
            {"y", pos.y}
          }},
          node->toJson().as_object(),
        });
      }
      json::object  value = {
        {"type", "SNT_FACTORY_NODE"},
        {"name", name},
        {"filepath", filepath},
        {"nodes", nodesJson},
      };
      return (value);
    }

    void            fromJson(json::value value) override {
      json::object& obj = value.as_object();
      name = obj["name"].as_string();
      filepath = obj["filepath"].as_string();
      for (auto& node: obj["nodes"].as_array()) {
        ImVec2  pos(node.as_object()["pos"].as_object()["x"].as_int64(), node.as_object()["pos"].as_object()["y"].as_int64());
        switch(sntFromString(node.as_object()["type"].as_string().c_str())) {
          case SNT_IN_NODE:
            addNode<InputNode>(pos)->fromJson(node.as_object());
            break;
          case SNT_OUT_NODE:
            addNode<OutputNode>(pos)->fromJson(node.as_object());
            break;
          case SNT_PART_NODE:
            for (auto& part: *partsPtr) {
              if (part.name == node.as_object()["name"].as_string()) {
                addNode<PartNode>(pos, part)->fromJson(node.as_object());
                break;
              }
            }
            break;
          case SNT_RECIPE_NODE:
            {
              int id = node.as_object()["recipeId"].as_int64();
              if (recipesPtr->size() < id)
                addNode<RecipeNode>(pos, (*recipesPtr)[id])->fromJson(node.as_object());
            }
            break;
          case SNT_FACTORY_NODE:
            addNode<FactoryNode>(pos)->fromJson(node.as_object());
            break;
          default:
            break;
        }
      }
    }

    static std::vector<Part>*   partsPtr;
    static std::vector<Recipe>* recipesPtr;

  protected:
    std::string name = "";
    std::string filepath = "";
    ImNodeFlow  m_grid;
};

class FactoryEditor: public BaseNode {
  public:
    FactoryEditor() {};

    template<typename T, typename... Params>
    std::shared_ptr<T>   placeNode(Params&&... args) {
      std::shared_ptr<T>  node = m_grid.placeNode<T>(args...);
      return (node);
    }
    template<typename T, typename... Params>
    std::shared_ptr<T>  placeNodeAt(const ImVec2& pos, Params&&... args) {
      std::shared_ptr<T>  node = m_grid.placeNodeAt<T>(pos, args...);
      return (node);
    }
    template<typename T, typename... Params>
    std::shared_ptr<T>  addNode(const ImVec2& pos, Params&&... args) {
      std::shared_ptr<T>  node = m_grid.addNode<T>(pos, args...);
      return (node);
    }

    void  draw() override {
      m_grid.rightClickPopUpContent([this](BaseNode *node)
        {
          StatorNode* nodeStator = static_cast<StatorNode*>(node);
          if (node != nullptr) {
            ImGui::Text("%s", node->getName().c_str());
            if (ImGui::Button("Ok"))
              ImGui::CloseCurrentPopup();
            if (ImGui::Button("Delete Node")) {
              node->destroy();
              ImGui::CloseCurrentPopup();
            }
            nodeStator->drawPopUp();
          }
          else {
            if (ImGui::Button("Add Input node"))
              placeNode<InputNode>();
            if (ImGui::Button("Add Output node"))
              placeNode<OutputNode>();
          }
        });
      m_grid.droppedLinkPopUpContent([this](ImFlow::Pin *dragged)
          { dragged->deleteLink(); });


      std::vector<std::weak_ptr<ImFlow::Link>> links = m_grid.getLinks();
      ImGui::Text("Nodes: %u", m_grid.getNodesCount());
      ImGui::SameLine();
      ImGui::Text("Links: %lu", links.size());

      for (auto link: links) {
        auto p = link.lock();
        if (p == nullptr)
          continue ;
        if (p->isHovered()) {
        }
        if (p->isSelected() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          ImFlow::Pin *right = p->right();
          ImFlow::Pin *left = p->left();
          right->deleteLink();
          left->deleteLink();
        }
      }
      m_grid.update();
    }

  protected:
    ImNodeFlow  m_grid;
};
