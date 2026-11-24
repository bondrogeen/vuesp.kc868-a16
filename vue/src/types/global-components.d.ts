import type {
  VButton,
  VSelect,
  VLoader,
  VDropdown,
  VList,
  VTextField,
  VWifiIcon,
  VIconLogoMini,
  VIconLogo,
  VDialog,
} from 'vuesp-components/components';

declare module 'vue' {
  export interface GlobalComponents {
    VButton: typeof VButton;
    VSelect: typeof VSelect;
    VLoader: typeof VLoader;
    VDropdown: typeof VDropdown;
    VList: typeof VList;
    VTextField: typeof VTextField;
    VWifiIcon: typeof VWifiIcon;
    VIconLogoMini: typeof VIconLogoMini;
    VIconLogo: typeof VIconLogo;
    VDialog: typeof VDialog;
  }

  // Common ARIA attributes are valid on any component; the UI kit does not
  // declare them as props, so they are allowed globally.
  export interface ComponentCustomProps {
    'aria-label'?: string;
  }
}

export {};
