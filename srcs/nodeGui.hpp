#pragma once

#include "stator/stator.hpp"
#include <imgui.h>

class NodeGui: public BaseNode {
  public:
    NodeGui(): BaseNode() {};

    template<typename T, typename... Params>
    std::shared_ptr<T> addNode(const ImVec2& pos, Params&&... args) {
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
              m_grid.placeNode<InputNode>();
            if (ImGui::Button("Add Output node"))
              m_grid.placeNode<OutputNode>();
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

  private:
    ImNodeFlow  m_grid;
};
