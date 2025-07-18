/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2025 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "gui.h"
#include <fmt/printf.h>
#include <imgui.h>
#include "IconsFontAwesome4.h"
#include "imgui_internal.h"
#include "../ta-log.h"

void FurnaceGUI::drawMobileOrderSel() {
  if (!portrait) return;

  if (!orderScrollLocked) {
    if (orderScroll>(float)curOrder-0.005f && orderScroll<(float)curOrder+0.005f) {
      orderScroll=curOrder;
    } else if (orderScroll<curOrder) {
      orderScroll+=MAX(0.05f,(curOrder-orderScroll)*0.2f);
      if (orderScroll>curOrder) orderScroll=curOrder;
      WAKE_UP;
    } else {
      orderScroll-=MAX(0.05f,(orderScroll-curOrder)*0.2f);
      if (orderScroll<curOrder) orderScroll=curOrder;
      WAKE_UP;
    }
  }

  ImGui::SetNextWindowPos(ImVec2(0.0f,mobileMenuPos*-0.65*canvasH));
  ImGui::SetNextWindowSize(ImVec2(canvasW,0.12*canvasW));
  if (ImGui::Begin("OrderSel",NULL,globalWinFlags)) {
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImGuiWindow* window=ImGui::GetCurrentWindow();
    ImGuiStyle& style=ImGui::GetStyle();

    ImVec2 size=ImGui::GetContentRegionAvail();

    ImVec2 minArea=window->DC.CursorPos;
    ImVec2 maxArea=ImVec2(
      minArea.x+size.x,
      minArea.y+size.y
    );
    ImRect rect=ImRect(minArea,maxArea);
    ImGui::ItemSize(size,style.FramePadding.y);
    if (ImGui::ItemAdd(rect,ImGui::GetID("OrderSelW"))) {
      ImVec2 centerPos=ImLerp(minArea,maxArea,ImVec2(0.5,0.5));
      
      for (int i=0; i<e->curSubSong->ordersLen; i++) {
        ImVec2 pos=centerPos;
        ImVec4 color=uiColors[GUI_COLOR_TEXT];
        pos.x+=(i-orderScroll)*40.0*dpiScale;
        if (pos.x<-200.0*dpiScale) continue;
        if (pos.x>canvasW+200.0*dpiScale) break;
        String text=fmt::sprintf("%.2X",i);
        float targetSize=size.y-fabs(i-orderScroll)*2.0*dpiScale;
        if (targetSize<8.0*dpiScale) targetSize=8.0*dpiScale;
        color.w*=CLAMP(2.0f*(targetSize/size.y-0.5f),0.0f,1.0f);

        ImGui::PushFont(bigFont);
        ImVec2 textSize=ImGui::CalcTextSize(text.c_str());
        ImGui::PopFont();

        pos.x-=textSize.x*0.5*(targetSize/textSize.y);
        pos.y-=targetSize*0.5;

        dl->AddText(bigFont,targetSize,pos,ImGui::GetColorU32(color),text.c_str());
      }
    }
    if (ImGui::IsItemClicked()) {
      orderScrollSlideOrigin=ImGui::GetMousePos().x+orderScroll*40.0f*dpiScale;
      orderScrollRealOrigin=ImGui::GetMousePos();
      orderScrollLocked=true;
      orderScrollTolerance=true;
    }

    // time
    if (e->isPlaying() && settings.playbackTime) {
      int totalTicks=e->getTotalTicks();
      int totalSeconds=e->getTotalSeconds();
      String info="";

      if (totalSeconds==0x7fffffff) {
        info="∞";
      } else {
        if (totalSeconds>=86400) {
          int totalDays=totalSeconds/86400;
          int totalYears=totalDays/365;
          totalDays%=365;
          int totalMonths=totalDays/30;
          totalDays%=30;

          info+=fmt::sprintf("%dy",totalYears);
          info+=fmt::sprintf("%dm",totalMonths);
          info+=fmt::sprintf("%dd",totalDays);
        }

        if (totalSeconds>=3600) {
          info+=fmt::sprintf("%.2d:",(totalSeconds/3600)%24);
        }

        info+=fmt::sprintf("%.2d:%.2d.%.2d",(totalSeconds/60)%60,totalSeconds%60,totalTicks/10000);
      }

      ImVec2 textSize=ImGui::CalcTextSize(info.c_str());

      dl->AddRectFilled(ImVec2(11.0f*dpiScale,(size.y*0.5)-(5.0f*dpiScale)),ImVec2((21.0f*dpiScale)+textSize.x,(size.y*0.5)+textSize.y+(5.0f*dpiScale)),ImGui::GetColorU32(ImGuiCol_WindowBg));
      dl->AddRect(ImVec2(11.0f*dpiScale,(size.y*0.5)-(5.0f*dpiScale)),ImVec2((21.0f*dpiScale)+textSize.x,(size.y*0.5)+textSize.y+(5.0f*dpiScale)),ImGui::GetColorU32(ImGuiCol_Border),0,0,dpiScale);
      dl->AddText(ImVec2(16.0f*dpiScale,(size.y)*0.5),ImGui::GetColorU32(ImGuiCol_Text),info.c_str());
    }
  }
  ImGui::End();
}

#define NEXT_BUTTON \
  if (++buttonColumn>=buttonColumns) { \
    buttonColumn=0; \
  } else { \
    ImGui::SameLine(); \
  }

void FurnaceGUI::drawOrderButtons() {
  int buttonColumns=(settings.orderButtonPos==0)?8:1;
  int buttonColumn=0;

  while (buttonColumns<8 && ((int)(8/buttonColumns)*ImGui::GetFrameHeightWithSpacing())>ImGui::GetContentRegionAvail().y) {
    buttonColumns++;
  }

  if (ImGui::Button(ICON_FA_PLUS)) { handleUnimportant
    // add order row (new)
    doAction(GUI_ACTION_ORDERS_ADD);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(_("Add new order"));
  }
  NEXT_BUTTON;

  pushDestColor();
  if (ImGui::Button(ICON_FA_MINUS)) { handleUnimportant
    // remove this order row
    doAction(GUI_ACTION_ORDERS_REMOVE);
  }
  popDestColor();
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(_("Remove order"));
  } 
  NEXT_BUTTON;

  if (ImGui::Button(ICON_FA_FILES_O)) { handleUnimportant
    // duplicate order row
    doAction(GUI_ACTION_ORDERS_DUPLICATE);
  }
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
    doAction(GUI_ACTION_ORDERS_DEEP_CLONE);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(_("Duplicate order (right-click to deep clone)"));
  }
  NEXT_BUTTON;

  if (ImGui::Button(ICON_FA_ANGLE_UP)) { handleUnimportant
    // move order row up
    doAction(GUI_ACTION_ORDERS_MOVE_UP);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(_("Move order up"));
  }
  NEXT_BUTTON;

  if (ImGui::Button(ICON_FA_ANGLE_DOWN)) { handleUnimportant
    // move order row down
    doAction(GUI_ACTION_ORDERS_MOVE_DOWN);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(_("Move order down"));
  }
  NEXT_BUTTON;

  if (ImGui::Button(ICON_FA_ANGLE_DOUBLE_DOWN)) { handleUnimportant
    // duplicate order row at end
    doAction(GUI_ACTION_ORDERS_DUPLICATE_END);
  }
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
    doAction(GUI_ACTION_ORDERS_DEEP_CLONE_END);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(_("Place copy of current order at end of song (right-click to deep clone)"));
  }
  NEXT_BUTTON;

  if (ImGui::Button(changeAllOrders?ICON_FA_LINK"##ChangeAll":ICON_FA_CHAIN_BROKEN"##ChangeAll")) { handleUnimportant
    // whether to change one or all orders in a row
    changeAllOrders=!changeAllOrders;
  }
  if (ImGui::IsItemHovered()) {
    if (changeAllOrders) {
      ImGui::SetTooltip(_("Order change mode: entire row"));
    } else {
      ImGui::SetTooltip(_("Order change mode: one"));
    }
  }
  NEXT_BUTTON;

  if (orderEditMode==0 && mobileUI) {
    orderEditMode=1;
  }

  const char* orderEditModeLabel="?##OrderEditMode";
  if (orderEditMode==3) {
    orderEditModeLabel=ICON_FA_ARROWS_V "##OrderEditMode";
  } else if (orderEditMode==2) {
    orderEditModeLabel=ICON_FA_ARROWS_H "##OrderEditMode";
  } else if (orderEditMode==1) {
    orderEditModeLabel=ICON_FA_I_CURSOR "##OrderEditMode";
  } else {
    orderEditModeLabel=ICON_FA_MOUSE_POINTER "##OrderEditMode";
  }
  if (ImGui::Button(orderEditModeLabel)) { handleUnimportant
    orderEditMode++;
    if (orderEditMode>3) orderEditMode=mobileUI?1:0;
    curNibble=false;
  }
  if (ImGui::IsItemHovered()) {
    if (orderEditMode==3) {
      ImGui::SetTooltip(_("Order edit mode: Select and type (scroll vertically)"));
    } else if (orderEditMode==2) {
      ImGui::SetTooltip(_("Order edit mode: Select and type (scroll horizontally)"));
    } else if (orderEditMode==1) {
      ImGui::SetTooltip(_("Order edit mode: Select and type (don't scroll)"));
    } else {
      ImGui::SetTooltip(_("Order edit mode: Click to change"));
    }
  }
}

void FurnaceGUI::drawOrders() {
  static char selID[4096];
  if (nextWindow==GUI_WINDOW_ORDERS) {
    ordersOpen=true;
    ImGui::SetNextWindowFocus();
    nextWindow=GUI_WINDOW_NOTHING;
  }
  if (!ordersOpen) return;
  if (mobileUI) {
    patWindowPos=(portrait?ImVec2(0.0f,(mobileMenuPos*-0.65*canvasH)):ImVec2((0.16*canvasH)+0.5*canvasW*mobileMenuPos,0.0f));
    patWindowSize=(portrait?ImVec2(canvasW,canvasH-(0.16*canvasW)-(pianoOpen?(0.4*canvasW):0.0f)):ImVec2(canvasW-(0.16*canvasH),canvasH-(pianoOpen?(0.3*canvasH):0.0f)));
    ImGui::SetNextWindowPos(patWindowPos);
    ImGui::SetNextWindowSize(patWindowSize);
  } else {
    //ImGui::SetNextWindowSizeConstraints(ImVec2(440.0f*dpiScale,400.0f*dpiScale),ImVec2(canvasW,canvasH));
  }
  if (ImGui::Begin("Orders",&ordersOpen,globalWinFlags|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse,_("Orders"))) {
    if (ImGui::BeginTable("OrdColumn",(settings.orderButtonPos==0)?1:2,ImGuiTableFlags_BordersInnerV)) {
      if (settings.orderButtonPos==2) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthFixed);
      } else if (settings.orderButtonPos==1) {
        ImGui::TableSetupColumn("c0",ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("c1",ImGuiTableColumnFlags_WidthStretch);
      }

      ImVec2 prevSpacing=ImGui::GetStyle().ItemSpacing;
      if (settings.orderButtonPos!=0) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(1.0f*dpiScale,1.0f*dpiScale));
      }

      ImGui::TableNextRow();

      if (settings.orderButtonPos<2) {
        ImGui::TableNextColumn();
        drawOrderButtons();
      }

      if (settings.orderButtonPos==0) {
        ImGui::TableNextRow();
      }

      ImGui::TableNextColumn();

      int displayChans=0;
      for (int i=0; i<e->getTotalChannelCount(); i++) {
        if (e->curSubSong->chanShow[i]) displayChans++;
      }
      ImGui::PushFont(patFont);
      bool tooSmall=((displayChans+1)>((ImGui::GetContentRegionAvail().x)/(ImGui::CalcTextSize("AA").x+2.0*ImGui::GetStyle().ItemInnerSpacing.x)));
      float yHeight=ImGui::GetContentRegionAvail().y;
      float lineHeight=(ImGui::GetTextLineHeight()+4*dpiScale);
      if (e->isPlaying() || haveHitBounds) {
        if (followOrders) {
          float nextOrdScroll=(playOrder+1)*lineHeight-((yHeight-(tooSmall?ImGui::GetStyle().ScrollbarSize:0.0f))/2.0f);
          if (nextOrdScroll<0.0f) nextOrdScroll=0.0f;
          ImGui::SetNextWindowScroll(ImVec2(-1.0f,nextOrdScroll));
        }
      }
      ImVec2 clipBegin=ImGui::GetCursorScreenPos();
      ImVec2 clipEnd=clipBegin+ImGui::GetContentRegionAvail();
      if (ImGui::BeginTable("OrdersTable",1+displayChans,(tooSmall?ImGuiTableFlags_SizingFixedFit:ImGuiTableFlags_SizingStretchSame)|ImGuiTableFlags_ScrollX|ImGuiTableFlags_ScrollY)) {
        if (tooSmall) {
          // set up cell sizes? I don't know
        }
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,prevSpacing);
        ImGui::TableSetupScrollFreeze(1,1);
        ImGui::TableNextRow(0,lineHeight);
        ImGui::TableNextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text,uiColors[GUI_COLOR_ORDER_ROW_INDEX]);
        for (int i=0; i<e->getTotalChannelCount(); i++) {
          if (!e->curSubSong->chanShow[i]) continue;
          ImGui::TableNextColumn();
          ImGui::TextNoHashHide("%s",e->getChannelShortName(i));
        }
        // OH MY FREAKING. just let me sleep.
        clipBegin.y+=lineHeight;
        ImGui::PopStyleColor();
        for (int i=0; i<e->curSubSong->ordersLen; i++) {
          ImGui::TableNextRow(0,lineHeight);
          if (playOrder==i && e->isPlaying()) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,ImGui::GetColorU32(uiColors[GUI_COLOR_ORDER_ACTIVE]));
          ImGui::TableNextColumn();
          if (curOrder==i) {
            if (ImGui::GetCurrentWindowRead()->ScrollbarY) {
              clipEnd.x-=ImGui::GetStyle().ScrollbarSize;
            }
            // draw a border
            ImGui::PushClipRect(clipBegin,clipEnd,false);
            ImDrawList* dl=ImGui::GetWindowDrawList();
            ImVec2 rBegin=ImGui::GetCursorScreenPos();
            rBegin.y-=ImGui::GetStyle().CellPadding.y;
            ImVec2 rEnd=ImVec2(clipEnd.x,rBegin.y+lineHeight);
            dl->AddRect(rBegin,rEnd,ImGui::GetColorU32(uiColors[GUI_COLOR_ORDER_SELECTED]),2.0f*dpiScale);
            ImGui::PopClipRect();
          }
          ImGui::PushStyleColor(ImGuiCol_Text,uiColors[GUI_COLOR_ORDER_ROW_INDEX]);
          bool highlightLoop=(i>=loopOrder && i<=loopEnd);
          if (highlightLoop) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,ImGui::GetColorU32(uiColors[GUI_COLOR_SONG_LOOP]));
          if (settings.orderRowsBase==1) {
            snprintf(selID,4096,"%.2X##O_S%.2x",i,i);
          } else {
            snprintf(selID,4096,"%d##O_S%.2x",i,i);
          }
          if (ImGui::Selectable(selID)) {
            setOrder(i);
            curNibble=false;
            orderCursor=-1;

            if (orderEditMode==0) {
              handleUnimportant;
            }

            if (cursor.xCoarse==selStart.xCoarse && cursor.xFine==selStart.xFine && cursor.y==selStart.y && cursor.order==selStart.order &&
                cursor.xCoarse==selEnd.xCoarse && cursor.xFine==selEnd.xFine && cursor.y==selEnd.y && cursor.order==selEnd.order) {
              cursor.order=curOrder;
              selStart=cursor;
              selEnd=cursor;
            }
          }
          ImGui::PopStyleColor();
          for (int j=0; j<e->getTotalChannelCount(); j++) {
            if (!e->curSubSong->chanShow[j]) continue;
            if (!ImGui::TableNextColumn()) continue;
            DivPattern* pat=e->curPat[j].getPattern(e->curOrders->ord[j][i],false);
            /*if (!pat->name.empty()) {
              snprintf(selID,4096,"%s##O_%.2x_%.2x",pat->name.c_str(),j,i);
            } else {*/
              snprintf(selID,4096,"%.2X##O_%.2x_%.2x",e->curOrders->ord[j][i],j,i);
            //}

            ImGui::PushStyleColor(ImGuiCol_Text,(curOrder==i || e->curOrders->ord[j][i]==e->curOrders->ord[j][curOrder])?uiColors[GUI_COLOR_ORDER_SIMILAR]:uiColors[GUI_COLOR_ORDER_INACTIVE]);
            if (ImGui::Selectable(selID,settings.ordersCursor?(cursor.xCoarse==j && curOrder!=i):false)) {
              if (curOrder==i) {
                if (orderEditMode==0) {
                  prepareUndo(GUI_UNDO_CHANGE_ORDER);
                  e->lockSave([this,i,j]() {
                    if (changeAllOrders) {
                      for (int k=0; k<e->getTotalChannelCount(); k++) {
                        if (e->curOrders->ord[k][i]<(unsigned char)(DIV_MAX_PATTERNS-1)) e->curOrders->ord[k][i]++;
                      }
                    } else {
                      if (e->curOrders->ord[j][i]<(unsigned char)(DIV_MAX_PATTERNS-1)) e->curOrders->ord[j][i]++;
                    }
                  });
                  e->walkSong(loopOrder,loopRow,loopEnd);
                  makeUndo(GUI_UNDO_CHANGE_ORDER);
                } else {
                  orderCursor=j;
                  curNibble=false;
                }
              } else {
                setOrder(i);
                e->walkSong(loopOrder,loopRow,loopEnd);
                if (orderEditMode!=0) {
                  orderCursor=j;
                  curNibble=false;
                }

                // i wonder whether this is necessary
                if (cursor.xCoarse==selStart.xCoarse && cursor.xFine==selStart.xFine && cursor.y==selStart.y && cursor.order==selStart.order &&
                    cursor.xCoarse==selEnd.xCoarse && cursor.xFine==selEnd.xFine && cursor.y==selEnd.y && cursor.order==selEnd.order) {
                  cursor.order=curOrder;
                  selStart=cursor;
                  selEnd=cursor;
                }
              }

              if (orderEditMode==0) {
                handleUnimportant;
              }
            }
            ImGui::PopStyleColor();
            if (orderEditMode!=0 && curOrder==i && orderCursor==j) {
              // draw a border
              ImDrawList* dl=ImGui::GetWindowDrawList();
              dl->AddRect(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),ImGui::GetColorU32(uiColors[GUI_COLOR_TEXT]),2.0f*dpiScale);
            }
            if (!pat->name.empty() && ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s",pat->name.c_str());
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
              if (curOrder==i) {
                if (orderEditMode==0) {
                  prepareUndo(GUI_UNDO_CHANGE_ORDER);
                  e->lockSave([this,i,j]() {
                    if (changeAllOrders) {
                      for (int k=0; k<e->getTotalChannelCount(); k++) {
                        if (e->curOrders->ord[k][i]>0) e->curOrders->ord[k][i]--;
                      }
                    } else {
                      if (e->curOrders->ord[j][i]>0) e->curOrders->ord[j][i]--;
                    }
                  });
                  e->walkSong(loopOrder,loopRow,loopEnd);
                  makeUndo(GUI_UNDO_CHANGE_ORDER);
                } else {
                  orderCursor=j;
                  curNibble=false;
                }
              } else {
                setOrder(i);
                e->walkSong(loopOrder,loopRow,loopEnd);
                if (orderEditMode!=0) {
                  orderCursor=j;
                  curNibble=false;
                }

                if (cursor.xCoarse==selStart.xCoarse && cursor.xFine==selStart.xFine && cursor.y==selStart.y && cursor.order==selStart.order &&
                    cursor.xCoarse==selEnd.xCoarse && cursor.xFine==selEnd.xFine && cursor.y==selEnd.y && cursor.order==selEnd.order) {
                  cursor.order=curOrder;
                  selStart=cursor;
                  selEnd=cursor;
                }
              }
            }
          }
        }
        ImGui::PopStyleVar();
        ImGui::EndTable();
      }
      ImGui::PopFont();

      if (settings.orderButtonPos==2) {
        ImGui::TableNextColumn();
        drawOrderButtons();
      }

      if (settings.orderButtonPos!=0) {
        ImGui::PopStyleVar();
      }

      ImGui::EndTable();
    }
  }
  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) curWindow=GUI_WINDOW_ORDERS;
  ImGui::End();
}
